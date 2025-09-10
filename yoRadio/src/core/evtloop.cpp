/*
  This file is a part of yoRadio project
  https://github.com/vortigont/yoradio/

  a fork of yoRadio project from https://github.com/e2002/yoradio

  Copyright © 2025 Emil Muratov (Vortigont)
*/
#include <Arduino.h>
#include "esp32-hal.h"
#include "evtloop.h"

// LOGGING
#ifdef ARDUINO
#include "esp32-hal-log.h"
#else
#include "esp_log.h"
#endif

static const char* TAG = "evt";

ESP_EVENT_DEFINE_BASE(YO_CMD_EVENTS);
//ESP_EVENT_DEFINE_BASE(YO_GET_STATE_EVENTS);
ESP_EVENT_DEFINE_BASE(YO_NTF_STATE_EVENTS);
ESP_EVENT_DEFINE_BASE(YO_CHG_STATE_EVENTS);
ESP_EVENT_DEFINE_BASE(YO_MSGQ_EVENTS);

namespace evt {

#define LOOP_EVT_Q_SIZE         16             // events loop queue size
#define LOOP_EVT_PRIORITY       2              // task priority is a bit higher than arduino's loop()
#define LOOP_EVT_RUNNING_CORE   ARDUINO_RUNNING_CORE  //   tskNO_AFFINITY
#ifdef YO_DEBUG_LEVEL
 #define LOOP_EVT_STACK_SIZE     5120           // task stack size
#else
 #define LOOP_EVT_STACK_SIZE     4096           // task stack size
#endif

// LighEvents loop handler
static esp_event_loop_handle_t hndlr = nullptr;

void start(){
  if (hndlr) return;

  ESP_LOGI(TAG, "Cretating Event loop");

  esp_event_loop_args_t evt_cfg;
  evt_cfg.queue_size = LOOP_EVT_Q_SIZE;
  evt_cfg.task_name = "evt_loop";
  evt_cfg.task_priority = LOOP_EVT_PRIORITY;            // uxTaskPriorityGet(NULL) // same as parent
  evt_cfg.task_stack_size = LOOP_EVT_STACK_SIZE;
  evt_cfg.task_core_id = LOOP_EVT_RUNNING_CORE;

  //ESP_ERROR_CHECK(esp_event_loop_create(&levt_cfg, &loop_levt_h));
  esp_err_t err = esp_event_loop_create(&evt_cfg, &hndlr);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "event loop creation failed!");
  }
}

void stop(){ esp_event_loop_delete(hndlr); hndlr = nullptr; };

esp_event_loop_handle_t get_hndlr(){ return hndlr; }

void debug(){
    ESP_ERROR_CHECK( esp_event_handler_instance_register_with(hndlr, ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, debug_hndlr, NULL, nullptr) );
}

void debug_hndlr(void* handler_args, esp_event_base_t base, int32_t id, void* event_data){
  Serial.printf("evt tracker: %s:%d\n", base, id);
}


} // namespace evt
