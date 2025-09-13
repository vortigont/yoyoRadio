/*
  This file is a part of yoyoRadio project
  https://github.com/vortigont/yoyoRadio/

  a fork of yoRadio project from https://github.com/e2002/yoradio

  Copyright © 2025 Emil Muratov (Vortigont)
*/

#include "widget_controllers.hpp"
#include "core/evtloop.h"
#include "core/const_strings.h"
#include "core/log.h"

static constexpr const char* T_MQCtrl = "MQCtrl";


void SpectrumAnalyser_Controller::generate_cfg(JsonVariant cfg) const {
  cfg[P_type] = e2int(_unit->getVisType());
  cfg[T_amp] = _unit->getAmp();
  cfg[P_color] = _unit->getColors();
  cfg[T_fft_size] = _unit->getFFTsize();
}

/**
 * @brief load configuration from a json object
 * method should be implemented in derived class to process
 * class specific json object
 * @param cfg 
 */
void SpectrumAnalyser_Controller::load_cfg(JsonVariantConst cfg){
  //LOGD(T_WidgetMgr, println, "load cfg");
  //serializeJsonPretty(cfg, Serial);
  if (cfg.isNull()) return; // ignore empty object
  _unit->setVisType(static_cast<SpectrumAnalyser_Widget::visual_t>(cfg[P_type].as<int>()));
  _unit->setAmp(cfg[T_amp] | 4);
  _unit->setAvg(cfg[T_avg] | 0.8);
  _unit->setColors(cfg[P_color]);
  _unit->reset(cfg[T_fft_size] | 128 );
}

// *******************
// MessageQ_Controller

MessageQ_Controller::MessageQ_Controller(const char* label, const char* name_space, std::shared_ptr<AGFX_TextScroller> unit, int32_t qid, size_t qlen) :
  EmbUIUnit(label, name_space), _unit(unit), qid(qid), _max_q_len(qlen) {
  // create timer
  _tmr = xTimerCreate("MQ",
    pdMS_TO_TICKS(1000),
    pdFALSE,
    static_cast<void*>(this),
    [](TimerHandle_t h) { static_cast<MessageQ_Controller*>(pvTimerGetTimerID(h))->_evaluate_queue(); }
  );
}

MessageQ_Controller::~MessageQ_Controller(){
  // remove timer
  if (_tmr){
    xTimerStop(_tmr, portMAX_DELAY );
    xTimerDelete(_tmr, portMAX_DELAY );
  }
  _events_unsubsribe();
}

void MessageQ_Controller::_consume_msg(){
  // lock the pool
  std::lock_guard lock(msgPool.mtx);
  LOGV(T_MQCtrl, printf, "_consume_msg gid:%u, q size:%u\n", qid, msgPool._msg_list.size());

  // get into pool's message list. This is kind of dirty, think about do this in a more beautifull way
  for ( auto &m : msgPool._msg_list){
    // skip empty messages, I won't check group ID, it is either ours or ANY_GROUP that we must catch
    if (!m) // || (qid != ESP_EVENT_ANY_ID && m->qid != qid))
      continue;

    // a short cut - pick the message if currently nothing is scrolled at all
    if (!_current_msg){
      _current_msg.reset(m.release());
      _unit->begin(_current_msg->msg.c_str());
      continue;
    }

    if (m->id){
      // this is a tagged message, check if we have same to update it
      if (_current_msg && _current_msg->id == m->id){
        // worst case - we have to update current running message in thread-safe manner
        // for now let's use non-thread safe way and see the impact :))
        if (m->msg.length()){
          _current_msg.reset(m.release());
          _unit->update(_current_msg->msg.c_str());
        } else {
          // if new message is empty, abort current scroller
          _unit->abort();
          _current_msg.release();
          m.reset(nullptr);
        }
        continue;
      } else {
        auto id = m->id;
        // try to find a message with same id in our pool
        auto lookup = std::find_if(_mqueue.begin(), _mqueue.end(), [id](const message_t &itm){ return id == itm->id; });
        if (lookup != _mqueue.end()){
          // found a matching message in our queue, let's replace it with a new one
        if (m->msg.length()){
          (*lookup).reset(m.release());
        } else {
          // if new message is empty, then remove our message too
          _mqueue.erase(lookup);
          m.reset(nullptr);
        }

          // we consumed the message, could go on here
          continue;
        }
      // otherwise message with such id was not found, then it must be a new one, let's treat it as non-unique message further
      }
    }
    // non-tagged generic messages are just added to the end of the queue
    if (_mqueue.size() < _max_q_len){
      LOGD(T_MQCtrl, printf, "picked a new msg:%s\n", m->msg.c_str());
      _mqueue.emplace_back(m.release());
    }
    else
      m.reset(nullptr);    // if queue is full, then discard the message
  }
}

void MessageQ_Controller::_events_subsribe(){
  // command events
  esp_event_handler_instance_register_with(evt::get_hndlr(), YO_MSGQ_EVENTS, qid,
    [](void* self, esp_event_base_t base, int32_t id, void* data){ static_cast<MessageQ_Controller*>(self)->_events_msg_hndlr(reinterpret_cast<TextControlMessage*>(data)); },
    this, &_hdlr_msg_evt
  );

}

void MessageQ_Controller::_events_unsubsribe(){
  _unit->setCallBack(nullptr);
  esp_event_handler_instance_unregister_with(evt::get_hndlr(), YO_MSGQ_EVENTS, ESP_EVENT_ANY_ID, _hdlr_msg_evt);
}

void MessageQ_Controller::_events_msg_hndlr(TextControlMessage *cm){
  LOGD(T_MQCtrl, printf, "Q id:%d, event:%u\n", qid, cm->e);
  switch (cm->e){
    // new message arrived
    case evt::yo_event_t::newMsg :
      _consume_msg();
      break;
    
    case evt::yo_event_t::clearMsg : {
      // check if current message needs to be cleared
      if (_current_msg->id == cm->id){
        // let's do this in a thread-safe manner, set message's counter to 1, and it will be cleared on scroll_end event from scroller
        _current_msg->cnt = 1;
        _unit->abort();
      }
      // remove matching from the queue
      auto id = cm->id;
      std::erase_if(_mqueue, [id](const auto &m){ return (m->id == id); });
    }

    default:;
  }
}

void MessageQ_Controller::start(){
  _unit->setCallBack([this](CanvasTextScroller::event_t e)->bool { return _scroller_callback(e); });
  _events_subsribe();
};

bool MessageQ_Controller::_scroller_callback(CanvasTextScroller::event_t e){
  switch (e){
    // scrolling ended (now the string pointer can be changed in a thread-safe manner)
    case CanvasTextScroller::event_t::end :
      _scroll_ended();
      break;

    // string reached left egde, consider it end scrolling
    case CanvasTextScroller::event_t::tail_at_left :
      _unit->abort();
      break;

    default:;
  }
  return false;
}

void MessageQ_Controller::_scroll_ended(){
  LOGD(T_MQCtrl, printf, "Qid:%u, scroll end\n", qid);
  // persistent message, or decrement show counter and check if message needs to be requeued
  if (_current_msg->cnt == -1 || --_current_msg->cnt > 0){
    if (_current_msg->cnt > 0)
      --_current_msg->cnt;
    // a shortcut if queue is empty
    if (!_mqueue.size() && _current_msg->interval == 0){
      _unit->begin(_current_msg->msg.c_str());
      return;
    }
    // otherwise need to requeue message
    _current_msg->last_displayed = millis();
    _mqueue.emplace_back(_current_msg.release());
  } else {
    // or else discard current message and find next one in the queue
    _current_msg.reset(nullptr);
  }

  _evaluate_queue();
}

void MessageQ_Controller::_evaluate_queue(){
  // safety check - if _current_msg is not null then this call is missfired, just quit
  // or if Q is empty then nothing to do here
  if (_current_msg || !_mqueue.size())
    return;

  LOGD(T_MQCtrl, printf, "Qid:%u, pick next message\n", qid);
  // find a new message to scroll
  auto i = std::find_if(_mqueue.begin(), _mqueue.end(), [](const message_t& m){ return millis() - m->last_displayed >= m->interval;});
  // if found a message with valid interval time
  if (i != _mqueue.end()){
    _current_msg.reset((*i).release());
    _mqueue.erase(i);
    _unit->begin(_current_msg->msg.c_str());
  } else {
    // otherwise messages are pending deley interval, trigger timer to reevaluate it in a second
    xTimerStart(_tmr, portMAX_DELAY );
  }
}
