#ifndef dspcore_h
#define dspcore_h
#include "../core/options.h"
#include "gfx_lib.h"
#include "widgets/widgets.h"
#include "widgets/pages.h"




class DspCoreBase {
public:
  DspCoreBase(){}
  virtual ~DspCoreBase() = default;

  uint16_t plItemHeight, plTtemsCount, plCurrentPos;
  int plYStart;

  virtual void initDisplay() = 0;
  virtual void clearDsp(bool black=false) = 0;
  virtual void loop(bool force=false) = 0;

  virtual void printPLitem(uint8_t pos, const char* item, ScrollWidget& current) = 0;
  //virtual void printClock(uint16_t x, uint16_t y, /* have no idea what this is */ uint16_t timeheight, bool redraw) = 0;
  //virtual void clearClock() = 0;
  virtual void drawLogo(uint16_t top) = 0;
  virtual void drawPlaylist(uint16_t currentItem) = 0;


  static char* utf8Rus(const char* str, bool uppercase);
  static uint16_t textWidthGFX(const char *txt, uint8_t textsize);

  /**
   * @brief method should calculate char size for the DEFAULT font
   * whatever it is to be for specific display/driver/board
   * meant to work with monospace fonts only, needs to be refactored for UTF wariable width fonts
   * 
   * @param textsize 
   * @param width 
   * @param height 
   */
  virtual void charSize(uint8_t textsize, uint16_t& width, uint16_t& height);

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

  //virtual void writePixel(int16_t x, int16_t y, uint16_t color) = 0;
  //virtual void writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) = 0;
  virtual void setNumFont() = 0;

  virtual void flip(){};
  virtual void invert(){};
  virtual void sleep(){};
  virtual void wake(){};

  /**
   * @brief display locking
   * 
   * @param wait - block until lock could be aquired
   * @return true - if lock has been aquired
   * @return false - lock can't been aquired (non-blocking lock)
   */
  virtual bool lock(bool wait = true){ return true; };

  /**
   * @brief release lock
   * 
   */
  virtual void unlock(){};

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
  uint16_t _timewidth, _timeleft, _datewidth, _dateleft, _oldtimeleft, _oldtimewidth, _olddateleft, _olddatewidth, /* clockTop, clockRightSpace, clockTimeHeight,*/ _dotsLeft;

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


// abstact class based on ArduinoGFX extending it with drawing helpers
#ifdef _ARDUINO_GFX_H_
class DspCore_Arduino_GFX : public DspCoreBase, virtual public DISPLAY_ENGINE {

public:
  DspCore_Arduino_GFX(){};

  // Текст
  void gfxDrawText(int x, int y, const char* text, uint16_t color, uint16_t bgcolor, uint8_t size, const GFXfont* font = nullptr);
  void gfxDrawNumber(int x, int y, int num, uint16_t color, uint16_t bgcolor, uint8_t size, const GFXfont* font = nullptr);
  void gfxDrawFormatted(int x, int y, const char* fmt, uint16_t color, uint16_t bgcolor, uint8_t size, const GFXfont* font, ...);

  // Графика
  void gfxDrawPixel (int x, int y, uint16_t color) { drawPixel(x, y, color); };
  void gfxDrawLine  (int x0, int y0, int x1, int y1, uint16_t color) { drawLine(x0, y0, x1, y1, color); };
  void gfxDrawRect  (int x, int y, int w, int h, uint16_t color) { drawRect(x, y, w, h, color); };
  void gfxFillRect  (int x, int y, int w, int h, uint16_t color){ fillRect(x, y, w, h, color); };
  void gfxDrawBitmap(int x, int y, const uint16_t* bitmap, int w, int h){ draw16bitRGBBitmap(x, y, const_cast<uint16_t*>(bitmap), w, h); };

  // Очистка
  void gfxClearArea(int x, int y, int w, int h, uint16_t bgcolor){ fillRect(x, y, w, h, bgcolor); };
  void gfxClearScreen(uint16_t bgcolor){ fillScreen(bgcolor); };
  virtual void gfxFlushScreen(){};
//
//#ifdef FONT_DEFAULT_AGFX
  void setFont(const GFXfont* font = nullptr);
//#endif

//#ifdef FONT_DEFAULT_U8G2
  void setFont(const uint8_t* font);
//#endif
//

};
#endif    // _ARDUINO_GFX_H_


#if NEXTION_RX!=255 && NEXTION_TX!=255
  #define USE_NEXTION
  #include "../displays/nextion.h"
#endif





#ifndef DUMMYDISPLAY
  void loopDspTask(void * pvParameters);

class Display {

enum class state_t {
  empty = 0,
  bootlogo,
  normal,
  screensaver,
  na
};

  public:
    uint16_t currentPlItem;
    uint16_t numOfNextStation;
    displayMode_e _mode;

    Display() {};
    ~Display();
    displayMode_e mode() { return _mode; }
    void mode(displayMode_e m) { _mode=m; }
    void init();
    void loop();
    void _start();
    bool ready() { return _state == state_t::normal; }
    void resetQueue();
    void putRequest(displayRequestType_e type, int payload=0);
    void flip();
    void invert();
    bool deepsleep();
    void wakeup();
    void setContrast();
    void printPLitem(uint8_t pos, const char* item);

private:
    ScrollWidget _meta, _title1, _plcurrent;
    ScrollWidget *_weather;
    ScrollWidget *_title2;
    BitrateWidget *_fullbitrate;
    FillWidget *_metabackground, *_plbackground;
    SliderWidget *_volbar, *_heapbar;
    Pager _pager;
    Page _footer;
    VuWidget *_vuwidget;
    NumWidget _nums;
    ProgressWidget _testprogress;
    ClockWidget _clock;
    Page *_boot;
    TextWidget *_bootstring, *_volip, *_voltxt, *_rssi, *_bitrate;
    Ticker _returnTicker;
    state_t _state{state_t::empty};
    void _time(bool redraw = false);
    void _apScreen();
    void _swichMode(displayMode_e newmode);
    void _drawPlaylist();
    void _volume();
    void _title();
    void _station();
    void _drawNextStationNum(uint16_t num);
    void _createDspTask();
    void _showDialog(const char *title);
    void _buildPager();
    void _bootScreen();
    void _setReturnTicker(uint8_t time_s);
    void _layoutChange(bool played);
    void _setRSSI(int rssi);

    // event function handlers
    esp_event_handler_instance_t _hdlr_cmd_evt{nullptr};
    esp_event_handler_instance_t _hdlr_chg_evt{nullptr};

    /**
     * @brief subscribe to event mesage bus
     * 
     */
    void _events_subsribe();

    /**
     * @brief unregister from event loop
     * 
     */
    void _events_unsubsribe();

    // command events handler
    void _events_cmd_hndlr(int32_t id, void* data);

    // state change events handler
    void _events_chg_hndlr(int32_t id, void* data);
};

#else

class Display {
  public:
    uint16_t currentPlItem;
    uint16_t numOfNextStation;
    displayMode_e _mode;
  public:
    Display() {};
    displayMode_e mode() { return _mode; }
    void mode(displayMode_e m) { _mode=m; }
    void init();
    void _start();
    void putRequest(displayRequestType_e type, int payload=0);
    void loop(){}
    bool ready() { return true; }
    void resetQueue(){}
    void centerText(const char* text, uint8_t y, uint16_t fg, uint16_t bg){}
    void rightText(const char* text, uint8_t y, uint16_t fg, uint16_t bg){}
    void flip(){}
    void invert(){}
    void setContrast(){}
    bool deepsleep(){return true;}
    void wakeup(){}
    void printPLitem(uint8_t pos, const char* item){}
};

#endif


extern Display display;

// function that creates display controller class and assigns a pointer to DspCore* dsp, should be defined in one (and only one!) of the respective displayXXXX.cpp files
bool create_display_dev();


#endif    // dspcore_h
