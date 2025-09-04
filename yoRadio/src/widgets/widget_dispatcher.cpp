/*
  This file is a part of yoRadio project
  https://github.com/vortigont/yoyoRadio/

  a fork of yoRadio project from https://github.com/e2002/yoradio

  Copyright © 2025 Emil Muratov (Vortigont)
*/

#include "widget_dispatcher.hpp"
#include "locale/l10n.h"
#include "core/log.h"

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


void Widget_Dispatcher::begin(){
  LOGI(T_WidgetMgr, println, "Loading display widgets");

  // should start from a json, but for now let's implement static vector config
  _mpp.clear();
  root_page = _mpp.makePage();  // root page
  // create widgets as per bootstrap list
  for (const auto i : *_bootstrap){
    switch (hash_djb2a(i.wlabel)){
      case "bitrate"_sh :
        _spawn_static(yoyo_wdgt_t::bitrate, i.cfg);
        break;
      case "clock"_sh :
        _spawn_static(yoyo_wdgt_t::clock, i.cfg);
        break;
      case "stateHeader"_sh :
        _spawn_static(yoyo_wdgt_t::textStatic, i.cfg);
        break;
      case "scrollerStation"_sh :
        _spawn_static(yoyo_wdgt_t::textScroller, i.cfg);
        break;
      case "scrollerTitle"_sh :
        _spawn_static(yoyo_wdgt_t::textScroller, i.cfg);
        break;
      case "spectrumAnalyzer"_sh :
        _spawn_static(yoyo_wdgt_t::spectrumAnalyzer, i.cfg);
        break;
      default:;
    }
  }

  // this is not a real menu, so no need to activate the items
  //pageAutoSelect(root_page, some_id);
  // start menu from page mainmenu
  _mpp.menuStart(root_page);
}

void Widget_Dispatcher::start(std::string_view label){
  if (!label.length()) return;  // need specific label, we won't start with ALL objects

}

void Widget_Dispatcher::stop(std::string_view label){

}

void Widget_Dispatcher::spawn(std::string_view label){

}

void Widget_Dispatcher::_spawn_static(yoyo_wdgt_t unit, const void* cfg){
  LOGD(T_WidgetMgr, printf, "Static wdgt:%u\n", e2int(unit));

  switch (unit){
    // BitRate Widget
    case yoyo_wdgt_t::bitrate :
      _mpp.addMuippItem(new MuiItem_Bitrate_Widget(_mpp.nextIndex(), reinterpret_cast<const bitrate_box_cfg_t*>(cfg), _w, _h), root_page);
      break;

    // Clock
    case yoyo_wdgt_t::clock :
      _mpp.addMuippItem(new ClockWidget(_mpp.nextIndex(), *reinterpret_cast<const clock_cfg_t*>(cfg)->clk, *reinterpret_cast<const clock_cfg_t*>(cfg)->date), root_page);
      break;

    // Status title
    case yoyo_wdgt_t::textStatic : {
      auto u = std::make_shared<MuiItem_AGFX_StaticText>(
          _mpp.nextIndex(),
          device_state_literal.at(0) /* "idle" */,
          reinterpret_cast<const text_wdgt_t*>(cfg)->place.getAbsoluteXY(_w, _h),  // unwrap to real position
          reinterpret_cast<const text_wdgt_t*>(cfg)->style
        );
      _mpp.addMuippItem(u, root_page);
      }
      break;

    // Scroller
    case yoyo_wdgt_t::textScroller : {
      auto u = std::make_shared<MuiItem_AGFX_TextScroller>(
          _mpp.nextIndex(),
          reinterpret_cast<const scroller_cfg_t*>(cfg)->box.getBoxDimensions(_w, _h),  // unwrap into absolute position
          reinterpret_cast<const scroller_cfg_t*>(cfg)->scroll_speed,
          reinterpret_cast<const scroller_cfg_t*>(cfg)->style);
      _mpp.addMuippItem(u, root_page);
      }
      break;

    // Spectrum analyzer
    case yoyo_wdgt_t::spectrumAnalyzer :
      _mpp.addMuippItem(new SpectrumAnalyser_Widget(_mpp.nextIndex(), reinterpret_cast<const spectrum_box_cfg_t*>(cfg)->box, _w, _h), root_page);
      break;

    default:;
  }


}
