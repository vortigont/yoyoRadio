#pragma once

#include <mutex>
#define GFX_DEV_DEVICE JC3248W535       // this module uses AXS15231B display 
#include "dspcore.h"

#define BOOT_PRG_COLOR    0xE68B
#define BOOT_TXT_COLOR    0xFFFF
#define PINK              0xF97F

#define AXS15231B_SLPIN     0x10
#define AXS15231B_SLPOUT    0x11
#define AXS15231B_DISPOFF   0x28
#define AXS15231B_DISPON    0x29

Arduino_GFX* create_display_dev();

// Module device cointrol
class Dsp_JC3248W535 {
public:
  Dsp_JC3248W535();

  void sleep();
  void wake();
};
