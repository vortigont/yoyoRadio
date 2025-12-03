/*
  This file is a part of yoyoRadio project
  https://github.com/vortigont/yoyoRadio/

  a fork of yoRadio project from https://github.com/e2002/yoradio

  Copyright © 2025 Emil Muratov (Vortigont)
*/


#pragma once
#include <string>
#include <list>
#include <memory>
#include <mutex>
#include "evtloop.h"

#define SCROLLER_HEADER_GID  1
#define SCROLLER_TRACK_GID  2

/**
 * @brief just a text message with a bunch of control fields
 * 
 */
struct TextMessage {
  std::string msg;
  /**
   * @brief display counter
   * 0 - do not display (discard message)
   * -1 - display forever
   */
  int32_t cnt;
  // minimum interval between redisplaying message again, seconds. If zero - then cycle immediately
  int32_t interval;
  // unique message id, if 0 - then not a unique message
  uint32_t id;
  // message queue ID
  uint32_t qid;

  // last displayed time
  uint32_t last_displayed{0};
//  TextMessage() = default;
//  explicit TextMessage(const char* m, int32_t cnt = 1, int32_t interval = 0, uint32_t id = 0, uint32_t qid = 0) : msg(m), cnt(cnt), interval(interval), id(id), qid(qid) {}
//  explicit TextMessage(std::string&& m, int32_t cnt = 1, int32_t interval = 0, uint32_t id = 0, uint32_t qid = 0) : msg(m), cnt(cnt), interval(interval), id(id), qid(qid) {}
};

using TextMessage_pt = std::unique_ptr<TextMessage>;

/**
 * @brief control message that could be sent via event bus to manage consumer's queues
 * 
 */
struct TextControlMessage {
  evt::yo_event_t e;
  uint32_t id;
};

/**
 * @brief An intermediate queue between distributors and consumers
 * since ESP event bus does not work good with dynamic objects and pointers
 * I have to use an intermediate pipe that would hold messages when control plane is running via event bus
 * 
 */
class MessageQ_Controller;

/**
 * @brief Text Message pool
 * used as a shim between senders and consumers as ESP's event messaging does not work well with dynamic objects like std::strings
 * 
 */
class MessagePool {
  // @note list could contain null objects when those are consumed by requestors, so all dereferencing operations MUST chech for empty pointers first!
  std::list< TextMessage_pt > _msg_list;
  // declare our friends who can mangle with message list
  //friend class MessageQ_Controller;

  // Access Mutex
  std::mutex _mtx;

public:
  MessagePool() = default;


  // Add message to the pool
  void addMsg(TextMessage&& msg);

  // clears message with specified id/qid
  void clearMsg(uint32_t id, int32_t qid = ESP_EVENT_ANY_ID);

  /**
   * @brief Evict Message with specified Q ID
   * 
   * 
   * @param qid to look for, if ESP_EVENT_ANY_ID specified then any available message returned
   * @return TextMessage* matching message, nullptr if no messages found
   * @note you MUST control the lifetime of message object after eviction and destruct when not needed
   */
  TextMessage* evictMessage(int32_t qid);

private:
  // clears the queue from an empty messages consumed previously
  void _purge_voids(){
    std::lock_guard lock(_mtx);
    std::erase_if(_msg_list, [](const TextMessage_pt &m ){ return !m; });
  };
};
