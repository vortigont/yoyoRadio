#include "../core/options.h"
#if DSP_MODEL==DSP_JC1060P470
#include "display_JC1060P470C.h"
#include "display/Arduino_DSI_Display.h"
#include "fonts/bootlogo.h"
#include "../core/config.h"
#include "../core/log.h"

static Arduino_ESP32DSIPanel *dsipanel{nullptr};
static Arduino_DSI_Display *gfx{nullptr};
static Dsp_JC1060P470* dsp_dev{nullptr};

Arduino_GFX* create_display_dev(){
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
    DSP_WIDTH /* width */, DSP_HEIGHT /* height */, dsipanel, 0 /* rotation */, true /* auto_flush */,
    LCD_RST /* RST */, jd9165_init_operations, sizeof(jd9165_init_operations) / sizeof(lcd_init_cmd_t));

  return gfx;
}


Dsp_JC1060P470::Dsp_JC1060P470() {
  ledcAttach(TFT_BLK, 1000, 8);
  //ledcOutputInvert(TFT_BLK, true);
  ledcWrite(TFT_BLK, 200);
}

void Dsp_JC1060P470::sleep(void) { 
  Serial.println("DspCore::sleep");
  gfx->displayOff();
  ledcWrite(TFT_BLK, 0); // Выключаем подсветку через PWM
}

void Dsp_JC1060P470::wake(void) {
  Serial.println("DspCore::wake");
  gfx->displayOn();
  ledcWrite(TFT_BLK, map(config.store.brightness, 0, 100, 0, 255)); // Устанавливаем яркость через PWM
}

#endif  // DSP_MODEL==DSP_JC1060P470
