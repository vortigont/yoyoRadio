#ifndef displayAXS15231B_h
#define displayAXS15231B_h
#include "../core/options.h"

#include "Arduino.h"
#include "Arduino_GFX_Library.h"

#if CLOCKFONT_MONO
  //#include "fonts/DS_DIGI56pt7b_mono.h"        // https://tchapi.github.io/Adafruit-GFX-Font-Customiser/
  #include "fonts/DirectiveFour56.h"
  #include "fonts/DirectiveFour30.h"

#else
  #include "fonts/DS_DIGI56pt7b.h"
#endif
#include "tools/l10n.h"

#define CHARWIDTH   6
#define CHARHEIGHT  8

using Canvas = Arduino_Canvas;

#include "widgets/widgets.h"
#include "widgets/pages.h"

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

class DspCore: public Arduino_AXS15231B {
public:
    uint16_t plItemHeight;
    uint16_t plTtemsCount;
    uint16_t plCurrentPos;
    int plYStart;
#ifdef CPU_LOAD
    TextWidget cpuWidget;
#endif

    DspCore();
    void initDisplay();
    void drawLogo(uint16_t top);
    void clearDsp(bool black=false);
    void printClock();
    void printClock(uint16_t top, uint16_t rightspace, uint16_t timeheight, bool redraw);
    void clearClock();
    void drawPlaylist(uint16_t currentItem);
    void loop(bool force=false);
    void charSize(uint8_t textsize, uint8_t& width, uint16_t& height);
    void setTextSize(uint8_t s);
    uint16_t width();
    uint16_t height();
    void flip();
    void invert();
    void sleep();
    void wake();
    void setBrightness(uint8_t brightness);
    void writePixel(int16_t x, int16_t y, uint16_t color);
    void writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void setClipping(clipArea ca);
    void clearClipping();
    void setScrollId(void * scrollid) { _scrollid = scrollid; }
    void * getScrollId() { return _scrollid; }
    void setNumFont();
    uint16_t textWidth(const char *txt);
    uint16_t textWidthN(const char *txt, int n);
    uint16_t textWidthGFX(const char *txt, uint8_t textsize);
    void printPLitem(uint8_t pos, const char* item, ScrollWidget& current);
    void startWrite(void);
    void endWrite(void);
    uint8_t _charWidth(unsigned char c);
    uint32_t _calculateCpuUsage();
#ifndef BATTERY_OFF
    void readBattery();
#endif
private:
    char  _timeBuf[20], _dateBuf[20], _oldTimeBuf[20], _oldDateBuf[20], _bufforseconds[4], _buffordate[40];
    uint16_t _timewidth, _timeleft, _datewidth, _dateleft, _oldtimeleft, _oldtimewidth, _olddateleft, _olddatewidth, clockTop, clockRightSpace, clockTimeHeight, _dotsLeft;
    bool _clipping, _printdots;
    clipArea _cliparea;
    void * _scrollid;
    void _getTimeBounds();
    void _clockSeconds();
    void _clockDate();
    void _clockTime();
};

extern DspCore dsp;

#endif
