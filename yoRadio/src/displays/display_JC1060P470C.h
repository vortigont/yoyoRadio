#pragma once

//#include <mutex>
//  Display driver JD9165BA-DS
#define GFX_DEV_DEVICE JC1060P470
#include "dspcore.h"

#if __has_include("conf/display_1024x600_conf_custom.h")
  #include "conf/display_1024x600_conf_custom.h"
#else
  #include "conf/display_1024x600_conf.h"
#endif

#define BOOT_PRG_COLOR    0xE68B
#define BOOT_TXT_COLOR    0xFFFF


class DspCore : public DspCore_Arduino_GFX {
public:
  DspCore(Arduino_ESP32DSIPanel *p)
    : Arduino_DSI_Display(
      LCD_H_RES /* width */, LCD_V_RES /* height */,
      p,
      0 /* rotation */,
      true /* auto_flush */,
      LCD_RST /* RST */,
      jd9165_init_operations, sizeof(jd9165_init_operations) / sizeof(lcd_init_cmd_t))
    {
      if (!begin()) Serial.println("[JD9165BA] Failed to begin driver!");
    }


  void initDisplay() override;
  void clearDsp(bool black=false) override;

  void drawLogo(uint16_t top) override;
  void drawPlaylist(uint16_t currentItem) override;
  void printPLitem(uint8_t pos, const char* item, ScrollWidget& current) override;

  // mask ArduinoGFX's method
  void writePixelPreclipped(int16_t x, int16_t y, uint16_t color) override;
  void writeFillRectPreclipped(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override;

  void setNumFont() override;

  void flip() override { setRotation(config.store.flipscreen ? 2 : 0); }
  void invert() override { invertDisplay(config.store.invertdisplay); }
  void sleep() override;
  void wake() override;

  //void startWrite(void) override { _mtx.lock(); }
  //void endWrite(void) override { flush(); _mtx.unlock(); }

  void loop(bool force=false) override;

  // mask ArduinoGFX's method
  void writePixel(int16_t x, int16_t y, uint16_t color);

private:
  uint8_t _charWidth(unsigned char c) override;

#ifdef BATTERY_ON
  void readBattery();
#endif

};
  



//Arduino_Canvas *gfx = new Arduino_Canvas(320 /* width */, 480 /* height */, g, 0 /* output_x */, 0 /* output_y */, 0 /* rotation */);

