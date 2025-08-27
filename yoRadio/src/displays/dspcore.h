#pragma once
#include "../core/options.h"
#include "../core/common.h"
#include "Ticker.h"
#include "nextion.h"

#if __has_include("Arduino_GFX.h")
#include "Arduino_GFX.h"
#include "widgets/muipp_widgets.hpp"

#endif

/**
 * @brief abstract class to control hw features of a display device
 * i.e. brighness, sleep, touch, etc...
 */
class DisplayControl {
protected:
  // current brightness, default is 70%
  int32_t brt{70};
  // device brightness control, should be redefined in derived classes
  virtual void setDevBrightness(int32_t val) = 0;

  public:
  DisplayControl() = default;
  virtual ~DisplayControl(){};
  // init device control
  virtual void init();
  // set display brightness in a range of 0-100%
  void setBrightness(int32_t val);
  // low power mode enable/disable
  virtual void displaySuspend(bool state){ setBrightness( state ? 0 : brt); }
};

/**
 * @brief class that controls backlight through PWM and
 * uses ArduinoGFX's derivative to potentially put display onto/out of low power mode
 * 
 */
class DisplayControl_AGFX_PWM : public DisplayControl {
  int32_t _bcklight, _level;
  Arduino_GFX* _dev;
  int32_t _pwm_bit;
  uint32_t _duty;

public:
  DisplayControl_AGFX_PWM(int32_t backlight_gpio, int32_t level = HIGH, Arduino_GFX* device = nullptr, int32_t pwm_bit = 8, int32_t pwm_hz = 1000 );
  ~DisplayControl_AGFX_PWM(){ ledcDetach(_bcklight); }

  // control PWM brightness
  void setDevBrightness(int32_t val) override { ledcWrite(_bcklight, map(val, 0, 100, 0, (1 << _pwm_bit) - 1)); };    // map 0-100% to PWM's width
  // Display low power mode control
  void displaySuspend(bool state) override;
};

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
  Arduino_GFX*_gfx;
  DisplayControl* _dctrl;
  MuiPlusPlus _mpp;
  

  public:
    DisplayGFX(Arduino_GFX* gfx, DisplayControl* dctrl) : _gfx(gfx), _dctrl(dctrl) {};
    ~DisplayGFX();

    // initialize display (create device driver class)
    void init() override;

    // returns true if display has reached "main operational state" on boot
    bool ready() const { return _state == state_t::normal; }

    // flush msg Q
    void resetQueue() override;

    // send and event to display to process and draw specific component
    void putRequest(displayRequestType_e type, int payload=0);

    // control display brightness, range 0-100%
    void setBrightness(uint32_t v);

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
     * @brief Scroller #1 - for station name
     * 
     */
    std::shared_ptr<MuiItem_AGFX_TextScroller> _scroll_title1;

    /**
     * @brief Scroller #2 - for track title
     * 
     */
    std::shared_ptr<MuiItem_AGFX_TextScroller> _scroll_title2;

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
