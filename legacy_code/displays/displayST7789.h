#ifndef displayST7789_h
#define displayST7789_h


#include "Arduino.h"
//#include <Arduino_GFX_Library.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#if CLOCKFONT_MONO
  #include "fonts/DS_DIGI42pt7b_mono.h"        // https://tchapi.github.io/Adafruit-GFX-Font-Customiser/
#else
  #include "fonts/DS_DIGI42pt7b.h"
#endif
#include "locale/l10n.h"

#define CHARWIDTH   6
#define CHARHEIGHT  8

typedef GFXcanvas16 Canvas;
//typedef Arduino_Canvas Canvas;
#include "widgets/widgets.h"
#include "widgets/pages.h"

#if __has_include("conf/displayST7789conf_custom.h")
  #include "conf/displayST7789conf_custom.h"
#else
  #if DSP_MODEL==DSP_ST7789
    #include "conf/displayST7789conf.h"
  #else
    #include "conf/displayST7789_240conf.h"
  #endif
#endif

#define BOOT_PRG_COLOR    0xE68B
#define BOOT_TXT_COLOR    0xFFFF
#define PINK              0xF97F

//class DspCore: public Adafruit_ST7789 {
class DspCore: public  Arduino_ST7789 {
  public:
  //DspCore(Arduino_DataBus *b);
  #include "tools/commongfx.h"
};

//static Arduino_DataBus *bus{nullptr};
static DspCore* dsp{nullptr};

static bool create_display_dev(){
  if (dsp == nullptr ){
    dsp = new DspCore();
  }
  return dsp != nullptr;
}

#ifdef DISABLED_CODE
static bool create_display_dev(){
  if (bus == nullptr){

    #if DSP_HSPI
      bus = new Arduino_ESP32SPI(TFT_DC /* DC */, TFT_CS /* CS */, TFT_SCK /* SCK */, TFT_MOSI /* MOSI */, GFX_NOT_DEFINED /* MISO */, HSPI /* spi_num */);
    #else
      #ifdef TFT_CUSTOM_PINS
        bus = new Arduino_ESP32SPI(TFT_DC /* DC */, TFT_CS /* CS */, TFT_SCK /* SCK */, TFT_MOSI /* MOSI */, GFX_NOT_DEFINED /* MISO */);
      #else
        bus = new Arduino_ESP32SPI(TFT_DC /* DC */, TFT_CS /* CS */, SCK /* SCK */, MOSI /* MOSI */, GFX_NOT_DEFINED /* MISO */);
      #endif  // TFT_CUSTOM_PINS
    #endif
  }

  if (bus == nullptr){
    Serial.println("Can't create bus!");
    return false;
  }

  if (dsp == nullptr ){
    dsp = new DspCore(bus);
  }
  return dsp != nullptr;
}
#endif  //DISABLED_CODE

#endif
