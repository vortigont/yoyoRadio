#ifndef dspcore_h
#define dspcore_h
#include "../core/options.h"
#include "../core/common.h"
#include "gfx_lib.h"
#include "Ticker.h"
#include "nextion.h"
#ifdef _ARDUINO_GFX_H_
#include "widgets/muipp_widgets.hpp"
#endif

#ifdef NOT_NEEDED
/**
 * @brief Graphics API core display class
 * it providex a very simplified GFX api to wrap around other grphics libs
 * like AdafruitGFX, ArduinoGFX, etc...
 */
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
  //void setScrollId(void * scrollid) { _scrollid = scrollid; }
  void * getScrollId() { return _scrollid; }
  uint16_t textWidth(const char *txt);

protected:

  bool _clipping;
  clipArea _cliparea;
  void * _scrollid;
  virtual uint8_t _charWidth(unsigned char c) = 0;
};


// abstact class based on ArduinoGFX extending it with drawing helpers
#ifdef _ARDUINO_GFX_H_
/**
 * @brief Graphics API core display class
 * it adopts ArduinoGFX API
 */
//class DspCore_Arduino_GFX : public DspCoreBase, virtual public DISPLAY_ENGINE {
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
#endif // NOT_NEEDED

/**
 * @brief abstract display class (or better say output interface)
 * wraps around functional-based output devices,
 * i.e. screen, nextion, etc...
 */
class Display {

protected:
  enum class state_t {
    empty = 0,
    bootlogo,
    normal,
    screensaver,
    na
  };

public:
  // current playlist item
  uint16_t currentPlItem;
  // playlist index num for next station
  uint16_t numOfNextStation;
  // current display mode
  displayMode_e _mode;

  Display() = default;
  virtual ~Display(){};

  // initialize display (create device driver class, etc...)
  virtual void init() = 0;
  // send a control message (compat mode) 
  virtual void putRequest(displayRequestType_e type, int payload=0){};
  virtual bool ready() { return true; }

  // Dummy methods
  // flush msg Q
  virtual void resetQueue(){};
  virtual void flip(){};
  virtual void invert(){};
  virtual bool deepsleep(){ return true; };
  virtual void wakeup(){};
  virtual void setContrast(){};
  virtual void printPLitem(uint8_t pos, const char* item){};



  // get current display mode
  displayMode_e mode() const { return _mode; }
  // set current display mode
  void mode(displayMode_e m) { _mode=m; }


};

#ifdef _ARDUINO_GFX_H_
/**
 * @brief Graphics Display output device. i.e. screens  
 * 
 */
class DisplayGFX : public Display {
  // display graphics object
  Arduino_GFX* _gfx;
  MuiPlusPlus _mpp;
  

  public:
    DisplayGFX(Arduino_GFX* gfx) : _gfx(gfx) {};
    ~DisplayGFX();

    // initialize display (create device driver class)
    void init() override;

    // returns true if display has reached "main operational state" on boot
    bool ready() const { return _state == state_t::normal; }

    // flush msg Q
    void resetQueue() override;

    // send and event to display to process and draw specific component
    void putRequest(displayRequestType_e type, int payload=0);
//    void flip() override;
//    void invert() override;
//    bool deepsleep() override;
//    void wakeup() override;
//    void setContrast() override;
//    void printPLitem(uint8_t pos, const char* item) override;

private:
/*
    ScrollWidget _meta, _title1, _plcurrent;
    ScrollWidget *_weather;
    // string with player state - 'ready', 'connecting', etc...
    ScrollWidget *_title2;
    BitrateWidget *_fullbitrate;
    // Фоновые виджеты
    FillWidget *_metabackground, *_plbackground;
    // Volume bar
    SliderWidget *_volbar, *_heapbar;

    Pager _pager;
    Page _footer;
    // some pages container
    std::array<Page*,4> _pages;

    VuWidget *_vuwidget;
    NumWidget _nums;
    ProgressWidget _testprogress;
    ClockWidget _clock;
    // bootscreen
    Page *_boot;
    TextWidget *_bootstring, *_volip, *_voltxt, *_rssi, *_bitrate;
*/
    // ticker timer
    Ticker _returnTicker;
    state_t _state{state_t::empty};
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

    /**
     * @brief create and initialize widgets
     * 
     */
    void _start();


    void _loopDspTask();

    TaskHandle_t _dspTask{nullptr};
    QueueHandle_t _displayQueue{nullptr};

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

    // *************************************************
    // ************* MuiPP screen sets *****************

    // *** Main Screen ***
    // a set of widgets for Main screen (where radio plays)

    /**
     * @brief static text with device's status
     * i.e. 'mode', 'playback'...
     * 
     */
    MuiItem_pt _title_status;
    /**
     * @brief Scroller - for station name
     * 
     */
    std::shared_ptr<MuiItem_AGFX_TextScroller> _scroll_title1;

    void _build_main_screen();



};
#endif    // _ARDUINO_GFX_H_


/**
 * @brief just a blackhole stub, i.e. no display output device
 * 
 */
class DisplayDummy : public Display {
public:
    DisplayDummy() = default;

    void init() override {};
    void putRequest(displayRequestType_e type, int payload = 0) override;
};


/**
 * @brief class implenents Nextion displays management
 * 
 */
class DisplayNextion : public Display {
public:
  DisplayNextion() = default;

  void init() override { _nextion.begin(true); };
  void putRequest(displayRequestType_e type, int payload = 0) override;

private:
  Nextion _nextion;
  void _start();
  void _station();
};

extern Display* display;

// function that creates display interface controller class
// and respective device-specific object (DspCore* dsp), should be defined in one (and only one!) of the respective displayXXXX.cpp files
bool create_display();


#endif    // dspcore_h
