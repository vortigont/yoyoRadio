/*
  This file is a part of yoyoRadio project
  https://github.com/vortigont/yoyoRadio/

  a fork of yoRadio project from https://github.com/e2002/yoradio

  Copyright © 2025 Emil Muratov (Vortigont)
*/


#include "textmsgq.hpp"
#include "core/log.h"

/**
 * @brief max messages in a pool
 * I do not need it to be large 'cause consumers should pick the messages pretty fast and keep
 * it in local queues. In case pool overflow for any reason it would work as FIFO and the oldest
 * messages are discarded  
 * 
 */
#define MSG_POOL_MAX_SIZE 4


void MessagePool::addMsg(TextMessage&& msg){
  // do not accept empty messages, instead clear ones with same id if present
  if (msg.msg.empty() && msg.id){
    clearMsg(msg.id, msg.qid);
    return;
  }

  // clear voids first
  _purge_voids();

  { // lock list
    std::lock_guard lock(_mtx);

    // if queue is full, then remove the oldest message
    if (_msg_list.size() > MSG_POOL_MAX_SIZE ){
      LOGW("MSG Pool", println, "MessagePool overflow!");
      _msg_list.pop_front();
    }

    _msg_list.emplace_back(std::make_unique<TextMessage>(msg));
  }
  // send a notification via event bus
  evt::yo_event_t e{evt::yo_event_t::newMsg};
  EVT_POST_DATA(YO_MSGQ_EVENTS, _msg_list.back()->qid, &e, sizeof(e));
};

void MessagePool::clearMsg(uint32_t id, int32_t qid){
  std::lock_guard lock(_mtx);
  // clear local Q first, remove empty or matching messages
  std::erase_if(_msg_list, [id, qid](const auto &m){ return ( !m || (m->id == id && (qid == ESP_EVENT_ANY_ID || qid == m->qid)) ); });
  // send an event to clean message from consumer's queues
  TextControlMessage cm{evt::yo_event_t::clearMsg, id};
  EVT_POST_DATA(YO_MSGQ_EVENTS, qid, &cm, sizeof(cm));
}

TextMessage* MessagePool::evictMessage(int32_t qid){
  std::lock_guard lock(_mtx);
  auto m = std::find_if(_msg_list.begin(), _msg_list.end(), [qid](const TextMessage_pt& msg){ return (qid == ESP_EVENT_ANY_ID || qid == msg->qid); });
  if (m != _msg_list.end()){
    auto mptr = m->release();
    _msg_list.erase(m);
    return mptr;
  } else
    return nullptr;
}
