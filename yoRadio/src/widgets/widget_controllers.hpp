/*
  This file is a part of yoyoRadio project
  https://github.com/vortigont/yoyoRadio/

  a fork of yoRadio project from https://github.com/e2002/yoradio

  Copyright © 2025 Emil Muratov (Vortigont)
*/

/*
  Widget controllers - an EmbUI Units that stich MuiPP derived widget objects
  with configuration features and EmbUI
  are used to:
    - serialize/deserialize widgets configuration and config presets
    - provide Web-based controls for widget class
    - interact between WebUI and Widget
    - interact between event bus and Widget additionaly if needed

*/

#pragma once
#include "embui_units.hpp"
#include "muipp_widgets.hpp"
#include "core/textmsgq.hpp"
#include "freertos/timers.h"

class SpectrumAnalyser_Controller : public EmbUIUnit_Presets {
  std::shared_ptr<SpectrumAnalyser_Widget> _unit;

public:
  SpectrumAnalyser_Controller(const char* label, const char* name_space, std::shared_ptr<SpectrumAnalyser_Widget> unit) :
    EmbUIUnit_Presets(label, name_space), _unit(unit) {}

  // start or initialize unit
  void start() override {};

  // stop or deactivate unit without destroying it
  void stop() override {};

private:

  /**
   * @brief derived method should generate object's configuration into provided JsonVariant
   * 
   * @param cfg 
   * @return JsonVariantConst 
   */
  void generate_cfg(JsonVariant cfg) const override;

  /**
   * @brief load configuration from a json object
   * method should be implemented in derived class to process
   * class specific json object
   * @param cfg 
   */
  void load_cfg(JsonVariantConst cfg) override;

};

/**
 * @brief this controller implements text message queue that is displayed via MuiPP text scroller
 * it depends on MessagePool instance to consume new messages from and event bus
 */
class MessageQ_Controller : public EmbUIUnit {
  MessagePool& _pool;
  std::shared_ptr<AGFX_TextScroller> _unit;

public:
  /**
   * @brief Construct a new MessageQ_Controller object
   * 
   * @param label unit label
   * @param name_space namespace label
   * @param unit text scroller unit
   * @param qid message queue ID (message group), ESP_EVENT_ANY_ID - any group (catch-all queue)
   * @param qlen message queue length
   */
  MessageQ_Controller(const char* label, const char* name_space, MessagePool& pool, std::shared_ptr<AGFX_TextScroller> unit, int32_t qid = ESP_EVENT_ANY_ID, size_t qlen = 16);
  ~MessageQ_Controller();

  // start or initialize unit
  void start() override;

  // stop or deactivate unit without destroying it
  void stop() override { _events_unsubsribe(); };

  // Instance's queue ID
  const int32_t qid;

private:
  const size_t _max_q_len;
  // message queue
  std::list< TextMessage_pt > _mqueue;
  TextMessage_pt _current_msg;

  // queue timer
  TimerHandle_t _tmr = nullptr;

  esp_event_handler_instance_t _hdlr_msg_evt{nullptr};

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
  void _events_msg_hndlr(TextControlMessage *cm);

  /**
   * @brief derived method should generate object's configuration into provided JsonVariant
   * 
   * @param cfg 
   * @return JsonVariantConst 
   */
  void generate_cfg(JsonVariant cfg) const override {};

  /**
   * @brief load configuration from a json object
   * method should be implemented in derived class to process
   * class specific json object
   * @param cfg 
   */
  void load_cfg(JsonVariantConst cfg) override {};

  // consume messages from a pool
  void _consume_msg();

  bool _scroller_callback(CanvasTextScroller::event_t e);

  // crolling ended event, reevaluate current message
  void _scroll_ended();
  
  // reevaluate current messeage and start scrolling next one
  void _evaluate_queue();
};
