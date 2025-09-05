/*
  This file is a part of yoRadio project
  https://github.com/vortigont/yoyoRadio/

  a fork of yoRadio project from https://github.com/e2002/yoradio

  Copyright © 2025 Emil Muratov (Vortigont)
*/

#pragma once
#include "embui_units.hpp"
#include "widgets/muipp_widgets.hpp"

// widgets lables - a list of predefined widgets which have individual configuration
static constexpr const char* T_bitrate = "bitrate";
static constexpr const char* T_clock = "clock";
static constexpr const char* T_stateHeader = "stateHeader";
static constexpr const char* T_scrollerStation = "scrollerStation";
static constexpr const char* T_scrollerTitle = "scrollerTitle";
static constexpr const char* T_spectrumAnalyzer = "spectrumAnalyzer";

/**
 * @brief this vector maps widgets labels to it's class types,
 * i.e. several widgets could co-exist at the same time with different config or so...
 * 
 */
static const std::vector< std::pair<const char*, yoyo_wdgt_t> > widgets_map {
  {T_bitrate, yoyo_wdgt_t::bitrate},
  {T_clock, yoyo_wdgt_t::clock},
  {T_stateHeader, yoyo_wdgt_t::textStatic},
  {T_scrollerStation, yoyo_wdgt_t::textScroller},
  {T_scrollerTitle, yoyo_wdgt_t::textScroller},
  {T_spectrumAnalyzer, yoyo_wdgt_t::spectrumAnalyzer}
};


/*
  this class is intended to stich together MuiPlusPlus Items (graphics widgets)
  with EmbUI controls, manage screen pages creation and configuration loading.

*/
class Widget_Dispatcher : public EmbUI_Unit_Manager {
  // operating screen resolution
  int16_t _w, _h;
  // configuration vector for bootstraping when json config on FS is not available
  const std::vector<widget_cfgitem_t>* _bootstrap;

public:
  Widget_Dispatcher(int16_t w, int16_t h, const std::vector<widget_cfgitem_t>* bootstrap) : EmbUI_Unit_Manager("wdgts"), _w(w), _h(h), _bootstrap(bootstrap) {}
  ~Widget_Dispatcher(){ _events_subsribe(); }

  // Access MuiPlusPlus object
  MuiPlusPlus* getMuipp(){ return &_mpp; }

  /**
   * @brief try to load widgets layout and create dependent pages
   * on a clean systems it bootstraps from a static array with basic widgets and layout
   * 
   */
  void begin() override;

  /**
   * @brief Creates and loads unit
   * if label is not given, then start all units based on settings from NVRAM
   * 
   * @param label 
   */
  void start(std::string_view label = {}) override;

  /**
   * @brief Stop specific unit if it's instance does exist
   * 
   * @param label 
   */
  void stop(std::string_view label = {}) override;

private:
  // MuiPP container
  MuiPlusPlus _mpp;
  muiItemId root_page;

  // temporary objects till I will make a queue
  std::shared_ptr<MuiItem_AGFX_TextScroller> _scroll_title1, _scroll_title2;

  // event function handlers
  esp_event_handler_instance_t _hdlr_cmd_evt{nullptr};
  esp_event_handler_instance_t _hdlr_chg_evt{nullptr};
  
  /**
   * @brief subscribe to event mesage bus
   * 
   */
  void _events_subsribe();

  /**
   * @brief unregister from event loop
   * 
   */
  void _events_unsubsribe();

  // command events handler
  void _events_cmd_hndlr(int32_t id, void* data);

  // state change events handler
  void _events_chg_hndlr(int32_t id, void* data);

  /**
   * @brief spawn a new instance of a module with supplied config
   * used with configuration is suplied via webui for non existing modules
   * @param label 
   */
  void spawn(std::string_view label) override;

  // spawn unit with static bootstrap config
  void _spawn_static(yoyo_wdgt_t unit, const void* cfg, const char* lbl = NULL);

};