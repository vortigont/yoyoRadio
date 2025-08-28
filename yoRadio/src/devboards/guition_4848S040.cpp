#include "databus/Arduino_SWSPI.h"
#include "display/Arduino_RGB_Display.h"  // ArduinoGFX driver
#include "guition_4848S040.hpp"
//#include "core/log.h"

namespace Guition_4848S040 {

Arduino_GFX* create_display_dev(const display_t &cfg, Arduino_DataBus *bus){
//  delete bus; // destruct previous object, if any
//  bus = new Arduino_SWSPI(GFX_NOT_DEFINED /* DC */, cfg.cs, cfg.sck, cfg.sda /* MOSI */, GFX_NOT_DEFINED /* MISO */);

//  if (!drv)
//    drv = new Arduino_AXS15231B(bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */, false /* IPS */, cfg.w, cfg.h);

  // device specific controller
  //if (!dsp_dev)
  //  dsp_dev = new Dsp_JC3248W535(cfg.backlight);

  // gfx object
//  return new Arduino_Canvas(cfg.w, cfg.h, drv);
  return nullptr;
}


};  //  namespace Guition_4848S040 {
