#include "canvas/Arduino_Canvas.h"
#include "display/Arduino_AXS15231B.h"  // ArduinoGFX driver
#include "databus/Arduino_ESP32QSPI.h"
#include "display_JC3248W535.h"
#include "../core/config.h"
#include "../core/log.h"

namespace JC3248W535 {

Arduino_DataBus *bus{nullptr};
Arduino_AXS15231B *drv{nullptr};
Arduino_Canvas* gfx{nullptr};
Dsp_JC3248W535 *dsp_dev{nullptr};

Arduino_GFX* create_display_dev(const JC3248W535::display_t &cfg){
  if (bus == nullptr){
    bus = new Arduino_ESP32QSPI(cfg.cs, cfg.sck, cfg.sda0, cfg.sda1, cfg.sda2, cfg.sda3);
  }

  if (bus == nullptr){
    LOGE(T_Display, println, "Can't create GFX bus!");
    return nullptr;
  }
  if (!drv)
    drv = new Arduino_AXS15231B(bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */, false /* IPS */, cfg.w, cfg.h);

  // device specific controller
  if (!dsp_dev)
    dsp_dev = new Dsp_JC3248W535(cfg.backlight);

  // gfx object
  gfx = new Arduino_Canvas(cfg.w, cfg.h, drv);
  return gfx;
}

Dsp_JC3248W535::Dsp_JC3248W535(int32_t backlight_gpio) : _backlight_gpio(backlight_gpio) {
  if (_backlight_gpio > -1){
    // backlight
    ledcAttach(_backlight_gpio, 1000, 8);
    //ledcOutputInvert(TFT_BLK, true);
    ledcWrite(_backlight_gpio, 200);    // default brightness
  }
}

void Dsp_JC3248W535::sleep(){ 
  //Serial.println("DspCore::sleep");
  drv->displayOff();
  #ifdef TFT_BLK
  ledcWrite(TFT_BLK, 0); // Выключаем подсветку через PWM
  #endif
}

void Dsp_JC3248W535::wake(){
  //Serial.println("DspCore::wake");
  drv->displayOn();
  ledcWrite(_backlight_gpio, map(75, 0, 100, 0, 255)); // Set brightness to 75%
  //ledcWrite(_backlight_gpio, map(config.store.brightness, 0, 100, 0, 255)); // Устанавливаем яркость через PWM
}

};  //  namespace JC3248W535 {
//#endif
