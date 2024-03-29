/*
 *  SIGnatures Monitor and UNifier Daemon
 *  Copyright (C) 2016  Marco Leogrande
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <thread>
#include "lib/db_interface.h"
#include "lib/es_interface.h"
#include "lib/sync_queue.h"

namespace freud {
namespace lib {

class Dispatcher {
 public:
  Dispatcher(const Configurator &config, DBInterface *db, ElasticSearchInterface *es);
  ~Dispatcher();

  // this method transfers ownership of the pointer inside the
  // Dispatcher
  void msg_received(std::string *msg);

  // stop the dispacher
  void stop();
  void stop_and_wait();

 private:
  DBInterface *db_;
  ElasticSearchInterface *es_;
  SyncQueue<std::string> inbound_queue_;
  std::thread *worker_;
  uint64_t ts_last_warning_; // used to throttle some warnings printed by this class

  const bool cache_packets_in_db_;
  const bool send_packets_to_es_;

  void worker_fn();
  void wait();
};

} // namespace lib
} // namespace freud
