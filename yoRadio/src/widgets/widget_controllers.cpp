/*
  This file is a part of yoyoRadio project
  https://github.com/vortigont/yoyoRadio/

  a fork of yoRadio project from https://github.com/e2002/yoradio

  Copyright © 2025 Emil Muratov (Vortigont)
*/

#include "widget_controllers.hpp"
#include "core/const_strings.h"
#include "core/log.h"

void SpectrumAnalyser_Controller::generate_cfg(JsonVariant cfg) const {
  cfg[P_type] = e2int(_unit->getVisType());
  cfg[T_amp] = _unit->getAmp();
  cfg[P_color] = _unit->getColors();
  cfg[T_fft_size] = _unit->getFFTsize();
}

/**
 * @brief load configuration from a json object
 * method should be implemented in derived class to process
 * class specific json object
 * @param cfg 
 */
void SpectrumAnalyser_Controller::load_cfg(JsonVariantConst cfg){
  //LOGD(T_WidgetMgr, println, "load cfg");
  //serializeJsonPretty(cfg, Serial);
  if (cfg.isNull()) return; // ignore empty object
  _unit->setVisType(static_cast<SpectrumAnalyser_Widget::visual_t>(cfg[P_type].as<int>()));
  _unit->setAmp(cfg[T_amp] | 4);
  _unit->setAvg(cfg[T_avg] | 0.8);
  _unit->setColors(cfg[P_color]);
  _unit->reset(cfg[T_fft_size] | 128 );
}
