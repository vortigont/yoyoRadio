#include "../core/options.h"
#if DSP_MODEL==DSP_JC3248W535
#include "canvas/Arduino_Canvas.h"
#include "display/Arduino_AXS15231B.h"  // ArduinoGFX driver
#include "databus/Arduino_ESP32QSPI.h"
#include "display_JC3248W535.h"
//#include "fonts/bootlogo.h"
#include "../core/config.h"
//#include "../core/network.h"
//#include "tools/l10n.h"
#include "../core/log.h"

static Arduino_DataBus *bus{nullptr};
static Arduino_AXS15231B *drv{nullptr};
static Arduino_Canvas* gfx{nullptr};
static Dsp_JC3248W535 *dsp_dev{nullptr};

Arduino_GFX* create_display_dev(){
  if (bus == nullptr){
    bus = new Arduino_ESP32QSPI(TFT_CS, TFT_SCK, TFT_SDA0, TFT_SDA1, TFT_SDA2, TFT_SDA3);
  }

  if (bus == nullptr){
    LOGE(T_Display, println, "Can't create GFX bus!");
    return nullptr;
  }
  if (!drv)
    drv = new Arduino_AXS15231B(bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */, false /* IPS */, TFT_WIDTH, TFT_HEIGHT);

  // device specific controller
  if (!dsp_dev)
    dsp_dev = new Dsp_JC3248W535();

  // gfx object
  gfx = new Arduino_Canvas(TFT_WIDTH, TFT_HEIGHT, drv);
  return gfx;
}

Dsp_JC3248W535::Dsp_JC3248W535(){
  // backlight
  ledcAttach(TFT_BLK, 1000, 8);
  //ledcOutputInvert(TFT_BLK, true);
  ledcWrite(TFT_BLK, 200);    // default brightness
}

void Dsp_JC3248W535::sleep(){ 
  Serial.println("DspCore::sleep");
  drv->displayOff();
  #ifdef TFT_BLK
  ledcWrite(TFT_BLK, 0); // Выключаем подсветку через PWM
  #endif
}

void Dsp_JC3248W535::wake(){
  Serial.println("DspCore::wake");
  drv->displayOn();
  ledcWrite(TFT_BLK, map(config.store.brightness, 0, 100, 0, 255)); // Устанавливаем яркость через PWM
}

#endif
