/*
  This file is a part of EmbUI project
  https://github.com/vortigont/EmbUI

  Copyright © 2023-2025 Emil Muratov (vortigont)

  EmbUI is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  EmbUI is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with EmbUI.  If not, see <https://www.gnu.org/licenses/>.
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

class MessageQ_Controller;

/**
 * @brief Text Message pool
 * used as a shim between senders and consumers as ESP's event messaging does not work well with dynamic objects like std::strings
 * 
 */
class MessagePool {
  std::list< std::unique_ptr<TextMessage> > _msg_list;
  // declare our friends who can mangle with message list
  friend class MessageQ_Controller;

public:
  MessagePool() = default;

  // Access Mutex
  std::mutex mtx;

  // Add message to the pool
  void addMsg(TextMessage&& msg);

  /**
   * @brief access the pool as const
   * adding / removing messages is not allowed, but can iterate and steal message pointers
   * on each access pool is purged from invalidated messages first
   * @return const std::list< std::unique_ptr<TextMessage> >& 
   */
  const std::list< std::unique_ptr<TextMessage> > &getPool();

private:
  // clears the queue from an empty messages consumed previously
  void _purge_voids(){ std::erase_if(_msg_list, [](const std::unique_ptr<TextMessage> &m ){ return !m; }); };
};


// MessagePool object 
extern MessagePool msgPool;
