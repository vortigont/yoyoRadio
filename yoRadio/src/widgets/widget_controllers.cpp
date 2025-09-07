/*
  This file is a part of yoyoRadio project
  https://github.com/vortigont/yoyoRadio/

  a fork of yoRadio project from https://github.com/e2002/yoradio

  Copyright © 2025 Emil Muratov (Vortigont)
*/

#include "widget_controllers.hpp"
#include "core/log.h"

void generate_cfg(JsonVariant cfg){

}

/**
 * @brief load configuration from a json object
 * method should be implemented in derived class to process
 * class specific json object
 * @param cfg 
 */
void load_cfg(JsonVariantConst cfg){
  LOGD(T_WidgetMgr, println, "load cfg");
  serializeJsonPretty(cfg, Serial);

}
