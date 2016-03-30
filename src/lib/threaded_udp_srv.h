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

#include <thread>

namespace freud {
namespace lib {

class ThreadedUDPServer {
 public:
  ThreadedUDPServer();
  ~ThreadedUDPServer() = default;

  uint16_t start_listening();
  void stop_listening();

  uint16_t get_listening_port() const { return port_; }

 private:
  bool try_init_socket();
  bool try_bind_port();
  void keep_listening();

  std::thread *listener_;
  int fd_;
  uint16_t port_;
  bool shutting_down_;
};

} // namespace lib
} // namespace freud
