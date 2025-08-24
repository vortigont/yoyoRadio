#pragma once

#include "JC3248W535_pincfg.h"
#define GFX_DEV_DEVICE JC3248W535       // this module uses AXS15231B display 
#include "dspcore.h"
/*
#define BOOT_PRG_COLOR    0xE68B
#define BOOT_TXT_COLOR    0xFFFF
#define PINK              0xF97F

#define AXS15231B_SLPIN     0x10
#define AXS15231B_SLPOUT    0x11
#define AXS15231B_DISPOFF   0x28
#define AXS15231B_DISPON    0x29
*/
namespace JC3248W535 {

Arduino_GFX* create_display_dev(const JC3248W535::display_t &cfg);

// Module device cointrol
class Dsp_JC3248W535 {
  int32_t _backlight_gpio;
public:
  Dsp_JC3248W535(int32_t backlight_gpio = -1);
  
  void sleep();
  void wake();
};

}