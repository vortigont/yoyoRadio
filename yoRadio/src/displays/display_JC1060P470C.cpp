// Arduino_ESP32DSIPanel is available only on ESP32-P4
#include "display_JC1060P470C.h"

#if defined(ESP32) && (CONFIG_IDF_TARGET_ESP32P4)

#include "display/Arduino_DSI_Display.h"
#include "../core/config.h"
#include "../core/log.h"

namespace JC1060P470 {
  

Arduino_ESP32DSIPanel *dsipanel{nullptr};
Arduino_DSI_Display *gfx{nullptr};
Dsp_JC1060P470* dsp_dev{nullptr};

Arduino_GFX* create_display_dev(const JC1060P470::display_t &cfg){
  if (!dsipanel){
    dsipanel = new Arduino_ESP32DSIPanel(
      40 /* hsync_pulse_width */,
      160 /* hsync_back_porch */,
      160 /* hsync_front_porch */,
      10 /* vsync_pulse_width */,
      23 /*vsync_back_porch  */,
      12 /* vsync_front_porch */,
      48000000 /* prefer_speed */);
  }

  if (!dsipanel){
    Serial.println("Can't create DSI Palel");
    return nullptr;
  }

  // device specific controller
  if (!dsp_dev)
    dsp_dev = new Dsp_JC1060P470();

  gfx = new Arduino_DSI_Display(
    cfg.w /* width */, cfg.h /* height */, dsipanel, 0 /* rotation */, false /* auto_flush */,
    cfg.rst /* RST */, jd9165_init_operations, sizeof(jd9165_init_operations) / sizeof(lcd_init_cmd_t));

  return gfx;
}


Dsp_JC1060P470::Dsp_JC1060P470(int32_t backlight_gpio) : _backlight_gpio(backlight_gpio) {
  if (backlight_gpio > -1){
    ledcAttach(backlight_gpio, 1000, 8);
    //ledcOutputInvert(TFT_BLK, true);
    ledcWrite(_backlight_gpio, 200);
  }
}

void Dsp_JC1060P470::sleep(void) { 
  //Serial.println("DspCore::sleep");
  gfx->displayOff();
  ledcWrite(_backlight_gpio, 0); // Выключаем подсветку через PWM
}

void Dsp_JC1060P470::wake(void) {
  //Serial.println("DspCore::wake");
  gfx->displayOn();
  ledcWrite(_backlight_gpio, map(config.store.brightness, 0, 100, 0, 255)); // Устанавливаем яркость через PWM
}

};  //  namespace JC1060P470

#else   //#if defined(ESP32) && (CONFIG_IDF_TARGET_ESP32P4)
namespace JC1060P470 {
Arduino_GFX* create_display_dev(const JC1060P470::display_t &cfg){ return nullptr; }
}
#endif  //#if defined(ESP32) && (CONFIG_IDF_TARGET_ESP32P4)
