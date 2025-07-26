#pragma once

#include <mutex>
#define GFX_DEV_DEVICE JC3248W535       // this module uses AXS15231B display 
#include "dspcore.h"

#if __has_include("conf/displayAXS15231Bconf_custom.h")
  #include "conf/displayAXS15231Bconf_custom.h"
#else
  #include "conf/displayAXS15231Bconf.h"
#endif

#define BOOT_PRG_COLOR    0xE68B
#define BOOT_TXT_COLOR    0xFFFF
#define PINK              0xF97F

#define AXS15231B_SLPIN     0x10
#define AXS15231B_SLPOUT    0x11
#define AXS15231B_DISPOFF   0x28
#define AXS15231B_DISPON    0x29

class DspCore : public DspCore_Arduino_GFX {
public:
  DspCore(Arduino_G *g)
    : Arduino_Canvas(TFT_WIDTH /* width */, TFT_HEIGHT /* height */, g, 0 /* output_x */, 0 /* output_y */, 0 /* rotation */){ 
      if (!begin()) Serial.println("[AXS15231B] Failed to begin canvas!");
    }
  //DspCore(Arduino_DataBus *b)
  //  : Arduino_AXS15231B(b, GFX_NOT_DEFINED /* RST */, 0 /* rotation */, false /* IPS */, TFT_WIDTH, TFT_HEIGHT) { begin(); }

  void initDisplay() override;
  void clearDsp(bool black=false) override;

  void drawLogo(uint16_t top) override;
  void printClock(uint16_t top, uint16_t rightspace, uint16_t timeheight, bool redraw) override;
  void clearClock() override;
  void drawPlaylist(uint16_t currentItem) override;
  void printPLitem(uint8_t pos, const char* item, ScrollWidget& current) override;

  void writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override;
  void setNumFont() override;

  void flip() override { setRotation(config.store.flipscreen ? 2 : 0); }
  void invert() override { invertDisplay(config.store.invertdisplay); }
  void sleep() override;
  void wake() override;

  void startWrite(void) override { _mtx.lock(); }
  void endWrite(void) override { flush(); _mtx.unlock(); }

  void loop(bool force=false) override;

  // mask ArduinoGFX's method
  void writePixel(int16_t x, int16_t y, uint16_t color);

private:
  std::mutex _mtx;
  uint8_t _charWidth(unsigned char c) override;
  void _clockSeconds();
  void _clockDate();
  void _clockTime();
#ifdef BATTERY_ON
  void readBattery();
#endif

};
  



//Arduino_Canvas *gfx = new Arduino_Canvas(320 /* width */, 480 /* height */, g, 0 /* output_x */, 0 /* output_y */, 0 /* rotation */);

