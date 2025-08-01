#ifndef displayILI9225_h
#define displayILI9225_h
#include "../core/options.h"
//==================================================
#include "Arduino.h"
#include "../ILI9225Fix/TFT_22_ILI9225Fix.h"

#if CLOCKFONT_MONO
  #include "fonts/DS_DIGI28pt7b_mono.h"                          // https://tchapi.github.io/Adafruit-GFX-Font-Customiser/
#else
  #include "fonts/DS_DIGI28pt7b.h"
#endif
#include "tools/l10n.h"

#define CHARWIDTH   6
#define CHARHEIGHT  8

typedef GFXcanvas16 Canvas;
#include "widgets/widgets.h"
#include "widgets/pages.h"

#if __has_include("conf/displayILI9225conf_custom.h")
  #include "conf/displayILI9225conf_custom.h"
#else
  #include "conf/displayILI9225conf.h"
#endif

#define BOOT_PRG_COLOR    0xE68B
#define BOOT_TXT_COLOR    0xFFFF
#define PINK              0xF97F

class DspCore: public TFT_22_ILI9225 {
#include "tools/commongfx.h"
};

extern DspCore dsp;

/*
// moved from dspcore.h
  #if DSP_MODEL==DSP_ILI9225
    uint16_t width(void) { return (int16_t)maxX(); }
    uint16_t height(void) { return (int16_t)maxY(); }
    void drawRGBBitmap(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w, int16_t h);
    uint16_t print(const char* s);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void setFont(const GFXfont *f = NULL);
    void setFont(uint8_t* font, bool monoSp=false );
    void setTextColor(uint16_t fg, uint16_t bg);
    void setCursor(int16_t x, int16_t y);
    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
    uint16_t drawChar(uint16_t x, uint16_t y, uint16_t ch, uint16_t color = COLOR_WHITE);
  #endif

  #if DSP_MODEL==DSP_ILI9225
    uint16_t _bgcolor, _fgcolor;
    int16_t  _cursorx, _cursory;
    bool _gFont;
  #endif
*/

#endif
