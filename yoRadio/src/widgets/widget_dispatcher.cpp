/*
  This file is a part of yoRadio project
  https://github.com/vortigont/yoyoRadio/

  a fork of yoRadio project from https://github.com/e2002/yoradio

  Copyright © 2025 Emil Muratov (Vortigont)
*/

#include "nvs_handle.hpp"
#include "widget_dispatcher.hpp"
#include "EmbUI.h"
#include "locale/l10n.h"
#include "core/log.h"

static constexpr const char* T_magic = "magic";
static constexpr const char* T_baseline = "baseline";

// map widget label to known class types
yoyo_wdgt_t map_widget_lbl_to_type(const char* lbl){
  switch (hash_djb2a(lbl)){
    case "bitrate"_sh :
      return yoyo_wdgt_t::bitrate;
    case "clock"_sh :
      return yoyo_wdgt_t::clock;
    case "stateHeader"_sh :
      return yoyo_wdgt_t::textStatic;
    case "scrollerStation"_sh :
      return yoyo_wdgt_t::textScroller;
    case "scrollerTitle"_sh :
      return yoyo_wdgt_t::textScroller;
    case "spectrumAnalyzer"_sh :
      return yoyo_wdgt_t::spectrumAnalyzer;
    default:
      return yoyo_wdgt_t::unknown;
  }
}

Widget_Dispatcher::Widget_Dispatcher(const char* name_space, uint32_t baseline_ver, const std::vector<widget_cfgitem_t> &baseline, int16_t w, int16_t h) :
  EmbUI_Unit_Manager(name_space), _w(w), _h(h), _units_index_page(P_embuium_idx_page_tlp) {

  _units_index_page.replace(8, 3, ns);    // replace token in tempalte "embuium.%ns.list"

  // copy rom's baseline config
  _baseline = baseline;

  esp_err_t err;
  std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(ns, NVS_READWRITE, &err);
  if (err == ESP_OK) {
    // check saved magic num if it's matching supplied baseline
    uint32_t ver{0};
    handle->get_item(T_magic, ver);
    if (baseline_ver != ver){
      LOGD(T_WidgetMgr, println, "Loading updated baseline states");
      // magic does not match, must be an updated firmware or new profile, let's rewrite saved baseline states with supplied one
      handle->set_item(T_magic, baseline_ver);
      std::vector<uint8_t> state;
      state.reserve(_baseline.size());
      for (auto i : _baseline){
        state.push_back(i.enabled);
      }
      handle->set_blob(T_baseline, state.data(), state.size());
    } else {
      // magic matches, load baseline states vector from NVS, it contains "enable" states for widgets
      size_t size{0};
      err = handle->get_item_size(nvs::ItemType::BLOB_DATA, T_baseline, size);
      if (err == ESP_OK && size){
        LOGD(T_WidgetMgr, printf, "Loading baseline states from NVS: %u items\n", size);
        std::vector<uint8_t> state;
        // prefill the vector with empty structs, then read from NVS on top
        state.assign(size, 0);
        handle->get_blob(T_baseline, state.data(), size);
        size_t idx{0};
        // apply saved states to current baseline
        for (auto i : state){
          _baseline.at(idx++).enabled = i;
        }
      } else {
        LOGE(T_WidgetMgr, println, "Baseline from NVS is empy!");
      }
    }
  }
}

void Widget_Dispatcher::begin(){
  // should start from a json, but for now let's implement static vector config
  _mpp.clear();
  // root page
  root_page = _mpp.makePage();
  // create widgets as per bootstrap list
  LOGD(T_WidgetMgr, printf, "Load Baseline: %u wdgts\n", _baseline.size());
  for (const auto &i : _baseline){
    LOGD(T_WidgetMgr, printf, "Baseline wdgt:%u, state:%u\n", i.wtype, i.enabled);
    if (i.enabled)
      _spawn_wdgt(i);
    //_spawn_wdgt(i.wtype, i.wlabel, i.cfg);
  }

  // this is not a real menu, so no need to activate the items
  //pageAutoSelect(root_page, some_id);
  // start menu from page mainmenu
  _mpp.menuStart(root_page);
  _events_subsribe();
}

void Widget_Dispatcher::start(std::string_view label){
  auto i = std::find_if(_baseline.begin(), _baseline.end(), [label](const widget_cfgitem_t &w){ return label.compare(w.wlabel) == 0; });
  if (i == _baseline.end()) return;   // unknown label
  if (i->enabled)  return;            // instance is already running
  // create new instance
  _spawn_wdgt(*i);
  //_spawn_wdgt(i->wtype, i->wlabel, i->cfg);
  // set 'enable' state and save baseline to NVS
  i->enabled = true;
  _save_baseline_to_nvs();
}

void Widget_Dispatcher::stop(std::string_view label){
  LOGI(T_UnitMgr, printf, "Trying to stop %s widget\n", label.data());
  auto i = std::find_if(_baseline.begin(), _baseline.end(), [label](const widget_cfgitem_t &w){ return label.compare(w.wlabel) == 0; });
  if (i == _baseline.end()) return;   // unknown label
  if (!i->enabled)  return;           // instance is not running

  // find mapped mpp items to this unit instance and remove it from available pages,
  // widget might be uncontrollable and does not have a widget controller unit
  for (auto &m : _lbl2mpp){
    if (label.compare(m.lbl) == 0){
      _mpp.removeItem(m.id);
    }
  }

  // remove mapping
  std::erase_if(_lbl2mpp, [label](const lbl2mpp_map_t &m){ return label.compare(m.lbl) == 0; });

  // check if controller unit is spawned
  auto u = std::find_if(units.cbegin(), units.cend(), EmbUIUnit_MatchLabel<EmbUIUnit_pt>(label));
  if ( u != units.cend() ){
    LOGI(T_UnitMgr, printf, "deactivate %s\n", label.data()); // in general it's wrong to pass sv's data, but this sv was instantiated from a valid null-terminated string, so should be OK, just could have some unexpected trailing suffix
    // stop the Unit first
    (*u)->stop();
    // remove widget controller unit itself
    units.erase(u);
  }

  // remove state flag from NVS
  i->enabled = false;
  _save_baseline_to_nvs();
}

//void Widget_Dispatcher::_spawn_wdgt(yoyo_wdgt_t unit, const char* lbl, const void* cfg){
void Widget_Dispatcher::_spawn_wdgt(const widget_cfgitem_t &item){
  LOGD(T_WidgetMgr, printf, "Spawn wdgt:%u\n", e2int(item.wtype));

  switch (item.wtype){
    // BitRate Widget
    case yoyo_wdgt_t::bitrate :
      // create a mapping for new widget to bind it's mpp id to baseline's label
      _lbl2mpp.push_back({item.wlabel, _mpp.nextIndex()});
      //LOGD(T_WidgetMgr, printf, "Spawn bitrate:%u\n", _lbl2mpp.back().id);
      _mpp.addMuippItem(new MuiItem_Bitrate_Widget(_lbl2mpp.back().id, reinterpret_cast<const bitrate_box_cfg_t*>(item.cfg), _w, _h, item.wlabel), root_page);
      break;

    // Clock
    case yoyo_wdgt_t::clock : {
      _lbl2mpp.push_back({item.wlabel, _mpp.nextIndex()});
      //LOGD(T_WidgetMgr, printf, "Spawn clk:%u\n", _lbl2mpp.back().id);
      _mpp.addMuippItem(new ClockWidget(_lbl2mpp.back().id, reinterpret_cast<const clock_cfg_t*>(item.cfg)->clk, reinterpret_cast<const clock_cfg_t*>(item.cfg)->date), root_page);
      }
      break;

    // Status title
    case yoyo_wdgt_t::textStatic : {
      _lbl2mpp.push_back({item.wlabel, _mpp.nextIndex()});
      auto u = std::make_shared<MuiItem_AGFX_StaticText>(
          _lbl2mpp.back().id,
          device_state_literal.at(0) /* "idle" */,
          reinterpret_cast<const text_wdgt_t*>(item.cfg)->place.getAbsoluteXY(_w, _h),  // unwrap to real position
          reinterpret_cast<const text_wdgt_t*>(item.cfg)->style
        );
      _mpp.addMuippItem(u, root_page);
      }
      break;

    // Scroller
    case yoyo_wdgt_t::textScroller : {
      _lbl2mpp.push_back({item.wlabel, _mpp.nextIndex()});
      auto u = std::make_shared<MuiItem_AGFX_TextScroller>(
          _lbl2mpp.back().id,
          reinterpret_cast<const scroller_cfg_t*>(item.cfg)->box.getBoxDimensions(_w, _h),  // unwrap into absolute position
          reinterpret_cast<const scroller_cfg_t*>(item.cfg)->scroll_speed,
          reinterpret_cast<const scroller_cfg_t*>(item.cfg)->style,
          item.wlabel);
      _mpp.addMuippItem(u, root_page);
      // temp solution, till I make the Q
      if (std::string_view(item.wlabel).compare(T_scrollerStation))
        _scroll_title1 = u;
      if (std::string_view(item.wlabel).compare(T_scrollerTitle))
        _scroll_title2 = u;
      }
      break;

    // Spectrum analyzer
    case yoyo_wdgt_t::spectrumAnalyzer : {
      // create widget
      _lbl2mpp.push_back({item.wlabel, _mpp.nextIndex()});
      //LOGD(T_WidgetMgr, printf, "Spawn spect:%u\n", _lbl2mpp.back().id);
      auto w = std::make_shared<SpectrumAnalyser_Widget>(_lbl2mpp.back().id, reinterpret_cast<const spectrum_box_cfg_t*>(item.cfg)->box, _w, _h);
      _mpp.addMuippItem(w, root_page);
      }
      break;

    default:;
  }


}


void Widget_Dispatcher::_events_subsribe(){
  // subscripe to EmbUI actions
  EmbUI_Unit_Manager::setHandlers();

  // handler for generating units list page
  embui.action.add(_units_index_page.c_str(),
  [this](Interface *interf, JsonVariantConst data, const char* action){
    LOGD(T_WidgetMgr, println, "generate widgets list page");
    // load units list page from UI data
    interf->json_frame_interface();
    interf->json_section_uidata();
    interf->uidata_pick( _units_index_page.c_str() );
    // pull value frame with units status
    getUnitsStatuses(interf);
    interf->json_frame_flush();
  }
);


  // command events
/*
  esp_event_handler_instance_register_with(evt::get_hndlr(), YO_CMD_EVENTS, ESP_EVENT_ANY_ID,
    [](void* self, esp_event_base_t base, int32_t id, void* data){ static_cast<Widget_Dispatcher*>(self)->_events_cmd_hndlr(id, data); },
    this, &_hdlr_cmd_evt
  );
*/
  // state change events
  esp_event_handler_instance_register_with(evt::get_hndlr(), YO_CHG_STATE_EVENTS, ESP_EVENT_ANY_ID,
    [](void* self, esp_event_base_t base, int32_t id, void* data){ static_cast<Widget_Dispatcher*>(self)->_events_chg_hndlr(id, data); },
    this, &_hdlr_chg_evt
  );
}

void Widget_Dispatcher::_events_unsubsribe(){
  embui.action.remove(_units_index_page.c_str());
  //esp_event_handler_instance_unregister_with(evt::get_hndlr(), YO_CMD_EVENTS, ESP_EVENT_ANY_ID, _hdlr_cmd_evt);
  esp_event_handler_instance_unregister_with(evt::get_hndlr(), YO_CHG_STATE_EVENTS, ESP_EVENT_ANY_ID, _hdlr_chg_evt);
}

void Widget_Dispatcher::_events_cmd_hndlr(int32_t id, void* data){
  LOGV(T_WidgetMgr, printf, "cmd event rcv:%d\n", id);
  switch (static_cast<evt::yo_event_t>(id)){
    //case evt::yo_event_t::displayPStop :
    //  _layoutChange(false);
    //  break;

    default:;
  }
}

// notifications events
void Widget_Dispatcher::_events_chg_hndlr(int32_t id, void* data){
  LOGV(T_WidgetMgr, printf, "chg event rcv:%d\n", id);

  switch (static_cast<evt::yo_event_t>(id)){
/*
    // device mode change - update "title_status" widget (todo: this should be done from inside the widget)
    case evt::yo_event_t::devMode : {
      auto u = getUnitPtr(T_stateHeader);
      int32_t v = *static_cast<int32_t*>(data);
        if (u && v >= 0 && v < device_state_literal.size()){
          static_cast<MuiItem_AGFX_StaticText*>(u)->setName(device_state_literal.at(v));
        }

      }
    }
    break;
*/
    // new station title - update "title_status" widget
    case evt::yo_event_t::playerStationTitle : {
      // this is not thread-safe, to be fixed later (todo: this should be done from inside the widget)
      if (_scroll_title1)
        _scroll_title1->setText(static_cast<const char*>(data));
    }
    break;

    // new track title - update "title_status" widget
    case evt::yo_event_t::playerTrackTitle : {
      // this is not thread-safe, to be fixed later
      if (_scroll_title2)
        _scroll_title2->setText(static_cast<const char*>(data));
    }
    break;
    
    default:;
  }

}

void Widget_Dispatcher::_save_baseline_to_nvs(){
  esp_err_t err;
  std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(ns, NVS_READWRITE, &err);
  if (err == ESP_OK){
    std::vector<uint8_t> state;
    state.reserve(_baseline.size());
    for (auto &i : _baseline){
      state.push_back(i.enabled);
    }
    handle->set_blob(T_baseline, state.data(), state.size());
    LOGV(T_WidgetMgr, println, "saved state to NVS");
  }
}

void Widget_Dispatcher::getUnitsStatuses(Interface *interf) const {
  if (!_baseline.size()) return;

  interf->json_frame_value();
  // generate values with each unit's state started/not started
  // unit's state id format "set_embuium_{namespace}_{unit_lbl}_state"
  for ( auto i = _baseline.cbegin(); i != _baseline.cend(); ++i){
    if (i->enabled){
      std::string s(T_set_embuium_);
      s.append(ns);
      s.append(1, (char)0x5f);  // '_'
      s.append(i->wlabel);
      s.append(T__state);
      interf->value(s, true);
    }

  }
  // not needed
  //interf->json_frame_flush();
}
