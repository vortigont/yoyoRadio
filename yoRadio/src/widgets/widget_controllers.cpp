/*
  This file is a part of yoyoRadio project
  https://github.com/vortigont/yoyoRadio/

  a fork of yoRadio project from https://github.com/e2002/yoradio

  Copyright © 2025 Emil Muratov (Vortigont)
*/

#include "widget_controllers.hpp"
#include "core/const_strings.h"
#include "core/log.h"

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

void MessageQ_Controller::_consume_msg(){
  // lock the pool
  std::lock_guard lock(msgPool.mtx);
  LOGV(T_Scroller, printf, "_consume_msg gid:%u, q size:%u\n", qid, msgPool._msg_list.size());

  // get into pool's message list. This is kind of dirty, think about do this in a more beautifull way
  for ( auto &m : msgPool._msg_list){
    // skip empty and messages we are not interested in
    if (!m || (qid != 0 && m->qid != qid))
      continue;

    if (m->id){
      // this is a tagged message, check if we have same to update it
      if (_current_msg && _current_msg->id == m->id){
        // worst case - we have to update current running message in thread safe manner
        // for now let's use non-thread safe way and see the impact :))
        _current_msg.reset(m.release());
      } else {
        auto id = m->id;
        // try to find a message with same id in our pool
        auto lookup = std::find_if(_mqueue.begin(), _mqueue.end(), [id](const message_t &itm){ return id == itm->id; });
        if (lookup != _mqueue.end()){
          // found a matching message in our queue, let's replace it with a new one
          (*lookup).reset(m.release());
        } else {
          // such a message not found, let's try to add it to end of the queue
          if (_mqueue.size() < _max_q_len)
            _mqueue.emplace_back(m.release());
          else
            m.reset(nullptr);    // if queue is full, then discard the message
        }
      }
    } else {
      // non-tagged generic messages are just added to the end of the queue
      if (_mqueue.size() < _max_q_len)
        _mqueue.emplace_back(m.release());
      else
        m.reset(nullptr);    // if queue is full, then discard the message
    }
  }

  // check if currently nothing is scrolled and we could start scrolling newly consumed messages
  if (!_current_msg){
    _scroll_next_message();
  }
}

void MessageQ_Controller::_events_subsribe(){
  // command events
  esp_event_handler_instance_register_with(evt::get_hndlr(), YO_MSGQ_EVENTS, ESP_EVENT_ANY_ID,
    [](void* self, esp_event_base_t base, int32_t id, void* data){ static_cast<MessageQ_Controller*>(self)->_events_msg_hndlr(id, data); },
    this, &_hdlr_msg_evt
  );

}

void MessageQ_Controller::_events_unsubsribe(){
  _unit->setCallBack(nullptr);
  esp_event_handler_instance_unregister_with(evt::get_hndlr(), YO_MSGQ_EVENTS, ESP_EVENT_ANY_ID, _hdlr_msg_evt);
}

void MessageQ_Controller::_events_msg_hndlr(int32_t id, void* data){
  switch (static_cast<evt::yo_event_t>(id)){
    // new message arrived
    case evt::yo_event_t::newMsg :
      if (*reinterpret_cast<uint32_t*>(data) == qid || qid == 0){
        _consume_msg();
      }
      break;

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
      _scroll_next_message();
      break;

    // string reached left egde, consider it end scrolling
    case CanvasTextScroller::event_t::tail_at_left :
      _unit->abort();
      break;

    default:;
  }
  return false;
}

void MessageQ_Controller::_scroll_next_message(){
  // nothing to scroll 
  if (!_current_msg && !_mqueue.size()) return;

  LOGD(T_Scroller, printf, "Lookup for a next scroll message gid:%u\n", qid);

  // nothing is scrolling now, but has other messages in the pool
  if (!_current_msg && _mqueue.size()){
    // consume message from a Q
    _current_msg.reset(_mqueue.front().release());
    _mqueue.pop_front();
    _unit->begin(_current_msg->msg.c_str());
    //LOGI(T_Scroller, printf, "scroll mesg1: %s\n", _current_msg->msg.c_str());
    return;
  }

  // so now we have _current_msg that just finished it's scroll cycle, let's see what to do with this

  // persistent message, or decrement show counter and check if message needs to be requeued
  if (_current_msg->cnt == -1 || --_current_msg->cnt > 0){
    _current_msg->last_displayed = millis();
    _mqueue.emplace_back(_current_msg.release());
  }

  // need to find a new message to scroll
  auto i = std::find_if(_mqueue.begin(), _mqueue.end(), [](const message_t& m){ return millis() - m->last_displayed > m->interval;});
  // if found a message with valid interval time
  if (i != _mqueue.end()){
    _current_msg.reset((*i).release());
    _mqueue.erase(i);
  } else {
    // else - discard current message
    _current_msg.reset();
  }

  if (_current_msg){
    _unit->begin(_current_msg->msg.c_str());
    //LOGI(T_Scroller, printf, "scroll mesg2: %s\n", _current_msg->msg.c_str());
  }
}
