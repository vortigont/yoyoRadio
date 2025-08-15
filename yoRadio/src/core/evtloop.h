/*
  This file is a part of yoRadio project
  https://github.com/vortigont/yoradio/

  a fork of yoRadio project from https://github.com/e2002/yoradio

  Copyright © 2025 Emil Muratov (Vortigont)
*/
#pragma once
#include "esp_event.h"
#include "traits.hpp"     // embui's traits, needed for e2int()

// helper macro to reduce typing
#define EVT_POST(event_base, event_id) esp_event_post_to(evt::get_hndlr(), event_base, event_id, NULL, 0, portMAX_DELAY)
#define EVT_POST_DATA(event_base, event_id, event_data, data_size) esp_event_post_to(evt::get_hndlr(), event_base, event_id, event_data, data_size, portMAX_DELAY)

// ESP32 event loop defines
ESP_EVENT_DECLARE_BASE(YO_CMD_EVENTS);          // declaration of Yo setter Command events (in reply to this command, an YO_CHG_EVENTS could be generated)
ESP_EVENT_DECLARE_BASE(YO_GET_STATE_EVENTS);    // declaration of Yo "Get State" Command events - this is a request of some current states (in reply to this command, an YO_NTF_EVENTS could be generated)
ESP_EVENT_DECLARE_BASE(YO_NTF_STATE_EVENTS);    // declaration of Yo "Notify State" command events - this a current state reporting event (those events are published on request, not on change!!!)
ESP_EVENT_DECLARE_BASE(YO_CHG_STATE_EVENTS);    // declaration of Yo "Change State" notification events base (those events are published when some state changes or in reply to "cmd set" events)

// Radio's Event Loop
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

  // Device modes
  devMode = 30,             // set/notify about generic device mode changes, param - a member of yo_state enum. Dev modes has respective literal naming

  // Audio player
  // Audio player states
  playerStop = 100,         // player's state command/state, no param allowed
  playerPlay,               // player's state command/state, no param allowed
  playerPause,              // player's state command/state, no param allowed
  playerToggle,             // toggle Play/Stop state command/state, no param allowed
  playerPrev,               // previous track
  playerNext,               // next track
  plsStation,               // play radio station from a playlist, param: int n - index in a playlist entry
  playerMode = 110,         // player mode webradio/sdcard, param int n: 0 - for webradio, 1 - for SDCARD

  // Sound control
  audioVolume = 120,        // player's volume value command/state, param: int n
  audioVolumeStep,          // player's volume step incr/decr command, param: int n
  audioMute,
  audioUnMute,
  audioMuteToggle,
  audioBalance,             // Sound balance, param int8_t
  audioTone,                // Sound equlizer settings, param: equalizer_tone_t struct
  audioGetValues,           // request to notify of current audio settings (vol, balance, etc...), used in requests from WebUI, MQTT

  // Audio player metadata
  playerAudioInfo = 130,    // player notifies about current's data/stream meta - bitrate/codec, param audio_into_t
  playerStationTitle,       // player notifies about new station title, param - const char[] to the station name
  playerTrackTitle,         // player notifies about new track title, param - const char[] to the track name

  // Audio Playlist control

  // Display events
  displayBootstring = 200,
  displayClock,
  displayDrawPlaylist,
  displayDrawVol,
  displayNewIP,
  displayNewMode,
  displayNewStation,
  displayNewTitle,
  displayNewWeather,
  displayNextStation,
  displayNope,
  displayPStart,
  displayPStop,
  displaySDFileIndex,
  displayShowRSSI,
  displayShowVUMeter,
  displayShowWeather,
  displayStart,
  displayWait4SD,

  // ext devices control
  btnLock = 300,            // Lock button
  btnUnLock,                // UnLock button
  btnLockState,             // request for button lock state, ButtonEventHandler will reply with YO_STATE_EVENTS:: btnUnLock/btnLock

  encoderMode = 310,        // change event, which mode encoder has switched to, param: unsigned

  // Module manager
  modClk = 1000,             // Enable/disable Clock module. param: unsigned n, if zero, disable, other - enable
  modClkPreset,             // switch Clock preset, param: signed n, if negative switch to random profile
  modTxtScroller,           // Enable/disable Text module. param: unsigned n, if zero, disable, other - enable
  modTxtScrollerPreset,     // switch TextScroller preset, param: signed n, if negative switch to random profile


  noop_end                  // NoOp
};

/**
 * @brief generic device states
 * @note mnemonic strings for the states should be places in static array 'device_state_literal'
 * should be located in locale's header file
 * 
 */
enum class yo_state:int32_t {
  idle = 0,
  webstream

};

// structs that could be used as event payload

/**
 * @brief playing stream/data meta
 * 
 */
struct audio_into_t {
  uint32_t bitRate;
  const char *codecName;
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
