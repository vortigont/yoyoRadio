#ifndef dspcore_h
#define dspcore_h
#include "../core/options.h"

#if DSP_MODEL==DSP_DUMMY
  #define DUMMYDISPLAY
  #define DSP_NOT_FLIPPED
  #include "tools/l10n.h"
#elif DSP_MODEL==DSP_ST7735
  #include "displayST7735.h"
#elif DSP_MODEL==DSP_SSD1306 || DSP_MODEL==DSP_SSD1306x32
  #include "displaySSD1306.h"
#elif DSP_MODEL==DSP_NOKIA5110
  #include "displayN5110.h"
#elif DSP_MODEL==DSP_ST7789 || DSP_MODEL==DSP_ST7789_240
  #include "displayST7789.h"
#elif DSP_MODEL==DSP_SH1106
  #include "displaySH1106.h"
#elif DSP_MODEL==DSP_1602I2C || DSP_MODEL==DSP_2004I2C
  #include "displayLC1602.h"
#elif DSP_MODEL==DSP_SSD1327
  #include "displaySSD1327.h"
#elif DSP_MODEL==DSP_ILI9341
  #include "displayILI9341.h"
#elif DSP_MODEL==DSP_SSD1305 || DSP_MODEL==DSP_SSD1305I2C
  #include "displaySSD1305.h"
#elif DSP_MODEL==DSP_SH1107
  #include "displaySH1106.h"
#elif DSP_MODEL==DSP_1602 || DSP_MODEL==DSP_2004
  #include "displayLC1602.h"
#elif DSP_MODEL==DSP_GC9106
  #include "displayGC9106.h"
#elif DSP_MODEL==DSP_CUSTOM
  #include "displayCustom.h"
#elif DSP_MODEL==DSP_ILI9225
  #include "displayILI9225.h"
#elif DSP_MODEL==DSP_ST7796
  #include "displayST7796.h"
#elif DSP_MODEL==DSP_GC9A01A
  #include "displayGC9A01A.h"
#elif DSP_MODEL==DSP_ILI9488 || DSP_MODEL==DSP_ILI9486
  #include "displayILI9488.h"
#elif DSP_MODEL==DSP_SSD1322
  #include "displaySSD1322.h"
#elif DSP_MODEL==DSP_ST7920
  #include "displayST7920.h"
#elif DSP_MODEL== DSP_JC3248W535
  #include "display_JC3248W535.h"
#endif


class DspCoreBase {
public:
  DspCoreBase(){}
  virtual ~DspCoreBase(){}

  uint16_t plItemHeight, plTtemsCount, plCurrentPos;
  int plYStart;

  virtual void initDisplay() = 0;
  virtual void clearDsp(bool black=false) = 0;
  virtual void loop(bool force=false) = 0;
  //void printClock(){}

  virtual void printPLitem(uint8_t pos, const char* item, ScrollWidget& current) = 0;
  virtual void printClock(uint16_t top, uint16_t rightspace, uint16_t timeheight, bool redraw) = 0;
  virtual void clearClock() = 0;
  virtual void drawLogo(uint16_t top) = 0;
  virtual void drawPlaylist(uint16_t currentItem) = 0;


  static char* utf8Rus(const char* str, bool uppercase);


  virtual void charSize(uint8_t textsize, uint8_t& width, uint16_t& height);
  #ifndef DSP_LCD
    #if DSP_MODEL==DSP_NOKIA5110
      virtual void command(uint8_t c);
      virtual void data(uint8_t c);
    #else
      virtual void startWrite(void){};
      virtual void endWrite(void){};
    #endif
    //void setTextSize(uint8_t s);
  #else
    uint16_t width();
    uint16_t height();
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color){}
    void setTextSize(uint8_t s){}
    void setTextSize(uint8_t sx, uint8_t sy){}
    void setTextColor(uint16_t c, uint16_t bg){}
    void setFont(){}
    void apScreen();
  #endif

  virtual void writePixel(int16_t x, int16_t y, uint16_t color) = 0;
  virtual void writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) = 0;
  virtual void setNumFont() = 0;

  virtual void flip(){};
  virtual void invert(){};
  virtual void sleep(){};
  virtual void wake(){};

  void setClipping(clipArea ca);
  void clearClipping(){ _clipping = false; };
  void setScrollId(void * scrollid) { _scrollid = scrollid; }
  void * getScrollId() { return _scrollid; }
  uint16_t textWidth(const char *txt);

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

protected:
  char  _timeBuf[20], _dateBuf[20], _oldTimeBuf[20], _oldDateBuf[20], _bufforseconds[4], _buffordate[40];
  uint16_t _timewidth, _timeleft, _datewidth, _dateleft, _oldtimeleft, _oldtimewidth, _olddateleft, _olddatewidth, clockTop, clockRightSpace, clockTimeHeight, _dotsLeft;
  bool _clipping, _printdots;
  clipArea _cliparea;
  void * _scrollid;
  virtual void _getTimeBounds();
  void _clockSeconds();
  void _clockDate();
  void _clockTime();
  virtual uint8_t _charWidth(unsigned char c) = 0;
  #if DSP_MODEL==DSP_ILI9225
    uint16_t _bgcolor, _fgcolor;
    int16_t  _cursorx, _cursory;
    bool _gFont/*, _started*/;
  #endif



};




//extern DspCore dsp;

#endif
