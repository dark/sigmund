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

#include "lib/dispatcher.h"

namespace freud {
namespace lib {

Dispatcher::Dispatcher(const Configurator &config, DBInterface *db, ElasticSearchInterface *es)
    : db_(db), es_(es),
      cache_packets_in_db_(config.get_cache_packets_in_db()),
      send_packets_to_es_(config.get_send_packets_to_es()) {
  worker_ = new std::thread(&Dispatcher::worker_fn, this);
}

Dispatcher::~Dispatcher() {
  stop_and_wait();
}

void Dispatcher::msg_received(std::string *msg) {
  inbound_queue_.push(msg);
}

void Dispatcher::stop() {
  inbound_queue_.push(NULL);
}

void Dispatcher::stop_and_wait() {
  stop();
  wait();
}

void Dispatcher::worker_fn() {
  while (true) {
    std::string *s = inbound_queue_.pop_or_wait();
    if (!s)
      // stop processing events
      break;

    if (cache_packets_in_db_ && !db_->cache_packet(*s))
      fprintf(stderr, "WARNING: could not cache packet in database\n");

    if (send_packets_to_es_ && !es_->post_packet(*s))
      fprintf(stderr, "WARNING: could not post packet to ElasticSearch\n");

    delete s;
  }
}

void Dispatcher::wait() {
  std::thread *local_worker = worker_;
  worker_ = NULL;

  if (!local_worker)
    return;

  local_worker->join();
  delete local_worker;
}

} // namespace lib
} // namespace freud
