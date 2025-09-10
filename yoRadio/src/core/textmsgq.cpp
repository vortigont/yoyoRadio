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

#include "textmsgq.hpp"

/**
 * @brief max messages in a pool
 * I do not need it to be large 'cause consumers should pick the messages pretty fast and keep
 * it in local queues. In case pool overflow for any reason it would work as FIFO and the oldest
 * messages are discarded  
 * 
 */
#define MSG_POOL_MAX_SIZE 4

MessagePool msgPool;

void MessagePool::addMsg(TextMessage&& msg){
  std::lock_guard lock(msgPool.mtx);
  // clear voids first
  _purge_voids();
  if (_msg_list.size() > MSG_POOL_MAX_SIZE )
    _msg_list.pop_front();

  _msg_list.emplace_back(std::make_unique<TextMessage>(msg));
  // send a notification via event bus
  EVT_POST_DATA(YO_MSGQ_EVENTS, e2int(evt::yo_event_t::newMsg), &_msg_list.back()->qid, sizeof(TextMessage::qid));  
};

const std::list< std::unique_ptr<TextMessage> > &MessagePool::getPool(){
  // go over all elements and remove empty ones, i.e. leftovers after previous eviction calls
  _purge_voids();
  return _msg_list;
}
