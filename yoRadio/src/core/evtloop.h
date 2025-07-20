/*
  This file is a part of yoRadio project
  https://github.com/vortigont/yoradio/

  a fork of yoRadio project from https://github.com/e2002/yoradio

  Copyright © 2025 Emil Muratov (Vortigont)
*/
#pragma once
#include "esp_event.h"

// helper macro to reduce typing
#define EVT_POST(event_base, event_id) esp_event_post_to(evt::get_hndlr(), event_base, event_id, NULL, 0, portMAX_DELAY)
#define EVT_POST_DATA(event_base, event_id, event_data, data_size) esp_event_post_to(evt::get_hndlr(), event_base, event_id, event_data, data_size, portMAX_DELAY)

// ESP32 event loop defines
ESP_EVENT_DECLARE_BASE(YO_CMD_EVENTS);          // declaration of Yo setter Command events (in reply to this command, an YO_CHG_EVENTS could be generated)
ESP_EVENT_DECLARE_BASE(YO_GET_STATE_EVENTS);    // declaration of Yo "Get State" Command events - this is a request of some current states (in reply to this command, an YO_NTF_EVENTS could be generated)
ESP_EVENT_DECLARE_BASE(YO_NTF_STATE_EVENTS);    // declaration of Yo "Notify State" command events - this a current state reporting event (those events are published on request, not on change!!!)
ESP_EVENT_DECLARE_BASE(YO_CHG_STATE_EVENTS);    // declaration of Yo "Change State" notification events base (those events are published when some state changes or in reply to "cmd set" events)

// Lamp's Event Loop
namespace evt {


// YO events
enum class yo_event_t:int32_t {
  noop = 0,               // NoOp command
  // 0-9 are reserved for something extraordinary

  // **** Set/get state command events ****

  // lamp power and mode of operation
  pwr = 10,                 // get/set current power state, (optional) param int p: 0 - poweroff, 1 - poweron, 2 - pwrtoggle, 
  pwron,                    // switch power on
  pwroff,                   // switch power off
  pwrtoggle,                // power toggle

  // brightness control, parameter value - int
  brightness = 20,          // set/get brightness according to current scale, param: int n
  brightness_nofade,        // set brightness according to current scale and w/o fade effect
  brightness_lcurve,        // set brightness luma curve
  brightness_scale,         // set brightness scale
  brightness_step,          // step brightness incr/decr w/o fade, param: int n - step to shift
  gradualFade,              // start gradual brightness fade, param gradual_fade_t

  // Audio player
  plsStation = 40,          // play radio station from a playlist, param: int n - index in a playlist entry
  // Audio player states
  playerStop,               // player's state command/state, no param allowed
  playerPlay,               // player's state command/state, no param allowed
  playerPause,              // player's state command/state, no param allowed

  // ext devices control
  btnLock = 100,            // Lock button
  btnUnLock,                // UnLock button
  btnLockState,             // request for button lock state, ButtonEventHandler will reply with YO_STATE_EVENTS:: btnUnLock/btnLock

  encoderMode = 110,        // change event, which mode encoder has switched to, param: unsigned

  // Module manager
  modClk = 250,             // Enable/disable Clock module. param: unsigned n, if zero, disable, other - enable
  modClkPreset,             // switch Clock preset, param: signed n, if negative switch to random profile
  modTxtScroller,           // Enable/disable Text module. param: unsigned n, if zero, disable, other - enable
  modTxtScrollerPreset,     // switch TextScroller preset, param: signed n, if negative switch to random profile


  // **** state change / notification events ****
  fadeStart = 1000,
  fadeEnd,                  // param: signed int - brightness level that fader ended with


  noop_end                  // NoOp
};


  /**
   * @brief Starts Lamp's event loop task
   * this loop will manage events processing between Lamp's components and controls
   *
   * @return esp_event_loop_handle_t* a pointer to loop handle
   */
  void start();

  /**
   * @brief Stops Lamp's event loop task
   *
   * @return esp_event_loop_handle_t* a pointer to loop handle
   */
  void stop();

  esp_event_loop_handle_t get_hndlr();


  void debug();

  void debug_hndlr(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);

  /**
   * @brief data struct that defines gradual fade command
   * 
   */
  struct gradual_fade_t {
    int32_t fromB;        // start with this brightness level, if -1 - use current brightness
    int32_t toB;          // end with this brightness level, if -1 - use current brightness
    uint32_t duration;    // fade duration in ms
  };

} // namespace evt

// cast enum to int
template <class E>
constexpr std::common_type_t<int, std::underlying_type_t<E>>
e2int(E e) {
    return static_cast<std::common_type_t<int, std::underlying_type_t<E>>>(e);
}
