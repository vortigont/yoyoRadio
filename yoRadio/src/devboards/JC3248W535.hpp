#pragma once

#include "JC3248W535_pincfg.h"
#include "displays/dspcore.h"

namespace JC3248W535 {

Arduino_GFX* create_display_dev(const JC3248W535::display_t &cfg, Arduino_DataBus *bus);

// Module device cointrol
class Dsp_JC3248W535 : public DisplayControl {
  int32_t _backlight_gpio;
public:
  Dsp_JC3248W535(int32_t backlight_gpio);
};

}