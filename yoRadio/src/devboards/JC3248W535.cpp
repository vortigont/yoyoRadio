#include "canvas/Arduino_Canvas.h"
#include "display/Arduino_AXS15231B.h"  // ArduinoGFX driver
#include "databus/Arduino_ESP32QSPI.h"
#include "JC3248W535.hpp"
//#include "core/log.h"

namespace JC3248W535 {

Arduino_AXS15231B *drv{nullptr};
//Dsp_JC3248W535 *dsp_dev{nullptr};

Arduino_GFX* create_display_dev(const JC3248W535::display_t &cfg, Arduino_DataBus *bus){
  delete bus; // destruct previous object, if any
  bus = new Arduino_ESP32QSPI(cfg.cs, cfg.sck, cfg.sda0, cfg.sda1, cfg.sda2, cfg.sda3);
  if (!drv)
    drv = new Arduino_AXS15231B(bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */, false /* IPS */, cfg.w, cfg.h);

  // device specific controller
  //if (!dsp_dev)
  //  dsp_dev = new Dsp_JC3248W535(cfg.backlight);

  // gfx object
  return new Arduino_Canvas(cfg.w, cfg.h, drv);
}


};  //  namespace JC3248W535 {
//#endif
