#pragma once
#include "core/options.h"
#include "core/common.h"
#include "Ticker.h"
#include "nextion.h"
#include "Arduino_GFX.h"
#include "widgets/muipp_widgets.hpp"
#include "widgets/preset_common.hpp"

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
  virtual ~DisplayControl();
  // init device control
  virtual void init();
  // set display brightness in a range of 0-100%
  void setBrightness(int32_t val);
  // low power mode enable/disable
  virtual void displaySuspend(bool state){ setBrightness( state ? 0 : brt); }

protected:
  virtual void embui_publish();

private:
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

  // EmbUI handlers
  void _embui_actions_register();
  void _embui_actions_unregister();
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
  void setDevBrightness(int32_t val) override { ledcWrite(_bcklight, map(clamp(val, 0L, 100L), 0, 100, 0, (1 << _pwm_bit) - 1)); };    // map 0-100% to PWM's width
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
  virtual void printPLitem(uint8_t pos, const char* item){};

  // widgets features

  // load widgets preset, if supported by display
  virtual void load_main_preset(const std::vector<widget_cfgitem_t>& preset){};


  // get current display mode
  displayMode_e mode() const { return _mode; }
  // set current display mode
  void mode(displayMode_e m) { _mode=m; }


};

/**
 * @brief Graphics Display output device. i.e. screens  
 * 
 */
class DisplayGFX : public Display {
  // display graphics object
  Arduino_GFX *_gfx;
  MuiPlusPlus *_mpp;
  

public:
  DisplayGFX(Arduino_GFX *gfx, MuiPlusPlus *mpp) : _gfx(gfx), _mpp(mpp) {};
  ~DisplayGFX(){ _events_unsubsribe(); };

  /**
   * @brief initialize GFX display
   * creates display servicing task
   * 
   */
  void init() override;

private:
  state_t _state{state_t::empty};
  void _createDspTask();
  void _loopDspTask();

  TaskHandle_t _dspTask{nullptr};
  //QueueHandle_t _displayQueue{nullptr};

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


/**
 * @brief just a blackhole stub, i.e. no display output device
 * 
 */
class DisplayDummy : public Display {
public:
    DisplayDummy() = default;

    void init() override {};
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
