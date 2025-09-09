#include "nvs_handle.hpp"
#include "EmbUI.h"
#include "dspcore.h"
#include "locale/l10n.h"
#include "core/const_strings.h"
#include "core/evtloop.h"
#include "core/log.h"


#ifndef DSQ_SEND_DELAY
  #define DSQ_SEND_DELAY portMAX_DELAY
#endif

#define DISPLAY_GFX_TASK_PRIO     3
#define DISPLAY_TASK_STACK_SIZE   3072
#define DISPLAY_FPS_CAP           20
#define DISPLAY_ECO_FPS_CAP       10
#ifndef DSP_TASK_DELAY
  #define DSP_TASK_DELAY  pdMS_TO_TICKS(20)   // cap for 50 fps
#endif
// will use DSP_QUEUE_TICKS as delay interval for display task runner when there are no msgs in a queue to process
#define DSP_QUEUE_TICKS DSP_TASK_DELAY

// normal interval between display refresh
constexpr TickType_t display_delay_ticks = pdMS_TO_TICKS(1000 / DISPLAY_FPS_CAP);
// long interval between display refresh
constexpr TickType_t display_eco_delay_ticks = pdMS_TO_TICKS(1000 / DISPLAY_ECO_FPS_CAP);


void DisplayGFX::init() {
  _state = state_t::empty;
  if (_gfx->begin()){
    _gfx->fillScreen(0);
    _gfx->setUTF8Print(true);
  } else {
    LOGE(T_Display, println, "DisplayGFX.init FAILED!");
    return;
  }

  // create runner task
  xTaskCreatePinnedToCore(
    [](void* self){ static_cast<DisplayGFX*>(self)->_loopDspTask(); },
    "DispTask",
    DISPLAY_TASK_STACK_SIZE,
    static_cast<void*>(this),
    DISPLAY_GFX_TASK_PRIO,
    &_dspTask,
    CONFIG_ARDUINO_RUNNING_CORE);   // bind to Arduino's core

  _mode = PLAYER;
  _state = state_t::normal;

  _events_subsribe();
  LOGI(T_Display, println, "init OK!");
}

void DisplayGFX::_loopDspTask() {

  TickType_t xLastWakeTime = xTaskGetTickCount();                                                                                                             
  TickType_t delay_time = display_delay_ticks;

#if YO_DEBUG_LEVEL == 5
  static unsigned long fps1{0}, fps2{0}, fps_time = millis();
#endif  // YO_DEBUG_LEVEL == 5
  for(;;){
    // sleep to accomodate specified fps rate
    if ( xTaskDelayUntil( &xLastWakeTime, delay_time ) != pdTRUE ) {
      // if task was unable to sleep at all, then it can't keep up with desired rate or it was suspended for long
      // we can reset and skip this run effectively reducing refresh rate and giving other tasks time to work
      continue;
    }

    if (_redraw){
      // force redraw the entire screen
      _gfx->fillScreen(0);
      _mpp->render(_gfx);
      _gfx->flush();
    } else if (_mpp->refresh(_gfx)){
      // refresh screen items if needed
      _gfx->flush();
      #if YO_DEBUG_LEVEL == 5
      ++fps2;
      #endif  // YO_DEBUG_LEVEL == 5
    }
    #if YO_DEBUG_LEVEL == 5
    ++fps1;
    if ( millis() - fps_time > 1000){
      LOGV(T_Display, printf, "fps render/runs %u:%u\n", fps2, fps1);
      fps1 = fps2 = 0;
      fps_time = millis();
    }
    #endif  // YO_DEBUG_LEVEL == 5
  }
  vTaskDelete( NULL );
  _dspTask=NULL;
}

void DisplayGFX::_events_subsribe(){
  // command events
  esp_event_handler_instance_register_with(evt::get_hndlr(), YO_CMD_EVENTS, ESP_EVENT_ANY_ID,
  [](void* self, esp_event_base_t base, int32_t id, void* data){ static_cast<DisplayGFX*>(self)->_events_cmd_hndlr(id, data); },
  this, &_hdlr_cmd_evt
  );
/*
  // state change events
  esp_event_handler_instance_register_with(evt::get_hndlr(), YO_CHG_STATE_EVENTS, ESP_EVENT_ANY_ID,
  [](void* self, esp_event_base_t base, int32_t id, void* data){ static_cast<DisplayGFX*>(self)->_events_chg_hndlr(id, data); },
  this, &_hdlr_chg_evt
);
*/
}

void DisplayGFX::_events_unsubsribe(){
  //esp_event_handler_instance_unregister_with(evt::get_hndlr(), YO_CMD_EVENTS, ESP_EVENT_ANY_ID, _hdlr_cmd_evt);
  //esp_event_handler_instance_unregister_with(evt::get_hndlr(), YO_CHG_STATE_EVENTS, ESP_EVENT_ANY_ID, _hdlr_chg_evt);
}

void DisplayGFX::_events_cmd_hndlr(int32_t id, void* data){
  LOGV(T_Display, printf, "cmd event rcv:%d\n", id);
  switch (static_cast<evt::yo_event_t>(id)){
    case evt::yo_event_t::displayRedraw :
      _redraw = true;
      break;

    default:;
  }
}

// notifications events
void DisplayGFX::_events_chg_hndlr(int32_t id, void* data){
  LOGV(T_Display, printf, "chg event rcv:%d\n", id);

  switch (static_cast<evt::yo_event_t>(id)){
    // device mode change - update "title_status" widget (todo: this should be done from inside the widget)
    case evt::yo_event_t::devMode : {
    }
    break;

    default:;
  }

}


// ****************
//  DisplayControl methods
// ****************
DisplayControl::~DisplayControl(){
  _events_unsubsribe();
  _embui_actions_unregister();
}

void DisplayControl::init(){
  // load brightness value from nvs
  esp_err_t err;
  std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(T_Display, NVS_READONLY, &err);
  if (err != ESP_OK){
    // if no saved value exist then set bightness to 75% by default
    setDevBrightness(75);
  } else {
    handle->get_item(T_brightness, brt);
    setDevBrightness(brt);
  }
  // event bus
  _events_subsribe();
  // embui actions
  _embui_actions_register();
}

void DisplayControl::setBrightness(int32_t val){
  brt = clamp(val, 0L, 100L);   // limit to 100%
  LOGI(T_Display, printf, "setBrightness:%u\n", brt);
  setDevBrightness(brt);

  // save brightness value to nvs
  esp_err_t err;
  std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(T_Display, NVS_READWRITE, &err);
  if (err == ESP_OK){
    handle->set_item(T_brightness, brt);
  }
  // publish brightness change notification to event bus
  EVT_POST_DATA(YO_CHG_STATE_EVENTS, e2int(evt::yo_event_t::brightness), &val, sizeof(val));  
}

void DisplayControl::_events_subsribe(){
  // command events
  esp_event_handler_instance_register_with(evt::get_hndlr(), YO_CMD_EVENTS, ESP_EVENT_ANY_ID,
    [](void* self, esp_event_base_t base, int32_t id, void* data){ static_cast<DisplayControl*>(self)->_events_cmd_hndlr(id, data); },
    this, &_hdlr_cmd_evt
  );

  // state change events
  /*
  esp_event_handler_instance_register_with(evt::get_hndlr(), YO_CHG_STATE_EVENTS, ESP_EVENT_ANY_ID,
    [](void* self, esp_event_base_t base, int32_t id, void* data){ static_cast<DisplayControl*>(self)->_events_chg_hndlr(id, data); },
    this, &_hdlr_chg_evt
  );
*/
}

void DisplayControl::_events_unsubsribe(){
  esp_event_handler_instance_unregister_with(evt::get_hndlr(), YO_CMD_EVENTS, ESP_EVENT_ANY_ID, _hdlr_cmd_evt);
//  esp_event_handler_instance_unregister_with(evt::get_hndlr(), YO_CHG_STATE_EVENTS, ESP_EVENT_ANY_ID, _hdlr_chg_evt);
}

void DisplayControl::_events_cmd_hndlr(int32_t id, void* data){
  LOGD(T_Display, printf, "cmd event rcv:%d\n", id);
  switch (static_cast<evt::yo_event_t>(id)){

    // brightness control
    case evt::yo_event_t::displayBrightness :
      setBrightness(*reinterpret_cast<int32_t*>(data));
      break;

    // status request
    case evt::yo_event_t::reportStateAll :
      EVT_POST_DATA(YO_NTF_STATE_EVENTS, e2int(evt::yo_event_t::brightness), &brt, sizeof(brt));
      embui_publish();
    break;

    default:;
  }
}

void DisplayControl::_embui_actions_register(){
  // brightness control from EmbUI
  embui.action.add(T_disp_brt, [this](Interface *interf, JsonVariantConst data, const char* action){ setBrightness(data); } );
}

void DisplayControl::_embui_actions_unregister(){
  embui.action.remove(T_disp_brt);
}

void DisplayControl::embui_publish(){
  LOGD(T_Display, println, "brt publish");
  if (!embui.feeders.available()) return;
  // publish value to EmbUI feeds
  Interface interf(&embui.feeders);
  interf.json_frame_value();
  interf.value(T_disp_brt, brt);
  interf.json_frame_flush();
}


// ****************
//  DisplayControl_AGFX_PWM methods
// ****************
DisplayControl_AGFX_PWM::DisplayControl_AGFX_PWM(int32_t backlight_gpio, int32_t level, Arduino_GFX* device, int32_t pwm_bit, int32_t pwm_hz) : _bcklight(backlight_gpio), _level(level), _dev(device), _pwm_bit(pwm_bit) {
  if (_bcklight > -1){
    // backlight
    ledcAttach(_bcklight, pwm_hz, pwm_bit);
    if (!level)
      ledcOutputInvert(_bcklight, true);
  }
};

void DisplayControl_AGFX_PWM::displaySuspend(bool state){
  if (_dev){
    state ? _dev->displayOff() : _dev->displayOn();
  }
  if (state){
    // save current duty
    _duty = ledcRead(_bcklight);
    ledcWrite(_bcklight, 0);    // this ugly Arduino API does not allow to just pause PWM, todo: replace it with native IDF ledc API
  } else {
    ledcWrite(_bcklight, _duty);
  }
}


// ****************
//  Dummy display methods
// ****************


// ****************
//  Nextion display methods
// ****************
void DisplayNextion::_start(){
  //nextion.putcmd("page player");
  nextion.start();
  //config.setTitle(const_PlReady);
}

void DisplayNextion::putRequest(displayRequestType_e type, int payload){
  if(type==DSP_START) _start();
  requestParams_t request;
  request.type = type;
  request.payload = payload;
  nextion.putRequest(request);
}

void DisplayNextion::_station() {
  //nextion.newNameset(config.station.name);
  //nextion.bitrate(config.station.bitrate);
  nextion.bitratePic(ICON_NA);
}
    /*#ifdef USE_NEXTION
      nextion.newTitle(config.station.title);
    #endif*/
  /*#ifdef USE_NEXTION
    nextion.setVol(config.store.volume, _mode == VOL);
  #endif*/
      /*#ifdef USE_NEXTION
        char buf[50];
        snprintf(buf, 50, bootstrFmt, config.ssids[request.payload].ssid);
        nextion.bootString(buf);
      #endif*/
