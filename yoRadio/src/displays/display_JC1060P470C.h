#pragma once
#include "JC1060P470CIW_pincfg.h"

//  Display driver JD9165BA-DS
#include "dspcore.h"

namespace JC1060P470 {

// Arduino_ESP32DSIPanel is available only on ESP32-P4
#if defined(ESP32) && (CONFIG_IDF_TARGET_ESP32P4)

Arduino_GFX* create_display_dev(const JC1060P470::display_t &cfg);

// Module device control
class Dsp_JC1060P470 {
  int32_t _backlight_gpio;
public:
  Dsp_JC1060P470(int32_t backlight_gpio = -1);

};

#else   //#if defined(ESP32) && (CONFIG_IDF_TARGET_ESP32P4)

Arduino_GFX* create_display_dev(const JC1060P470::display_t &cfg);

#endif  //#if defined(ESP32) && (CONFIG_IDF_TARGET_ESP32P4)

};  //  namespace JC1060P470