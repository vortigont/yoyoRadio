#pragma once

//#include <mutex>
//  Display driver JD9165BA-DS
#define GFX_DEV_DEVICE JC1060P470
#include "dspcore.h"

#define BOOT_PRG_COLOR    0xE68B
#define BOOT_TXT_COLOR    0xFFFF

Arduino_GFX* create_display_dev();

// Module device cointrol
class Dsp_JC1060P470 {
public:
  Dsp_JC1060P470();

  void sleep();
  void wake();
};
