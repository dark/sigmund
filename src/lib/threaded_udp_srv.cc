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

#include "lib/threaded_udp_srv.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define UDP_FIRST_PORT 2377
#define UDP_LAST_PORT 2548

namespace freud {
namespace lib {

ThreadedUDPServer::ThreadedUDPServer()
    : listener_(NULL), fd_(0), port_(0) {
}

uint16_t ThreadedUDPServer::start_listening() {
  // listening socket already allocated
  if (port_)
    return port_;

  if (!try_init_socket())
    return 0;

  if (!try_bind_port())
    return 0;

  listener_ = new std::thread(keep_listening, this);
  return port_;
}

bool ThreadedUDPServer::try_init_socket() {
  if (fd_)
    // do not init twice
    return true;

  int local_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (local_fd < 0) {
    // failed to initialize the socket
    fprintf(stderr, "socket: %s\n", strerror(errno));
    return false;
  }

  // enable address reuse on socket
  int val = 1;
  if (::setsockopt(local_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
    // failed to initialize the socket
    fprintf(stderr, "setsockopt: %s\n", strerror(errno));
    return false;
  }

  fd_ = local_fd;
  return true;
}

bool ThreadedUDPServer::try_bind_port() {
  if (port_)
    return true;

  uint16_t local_port = UDP_FIRST_PORT;
  while (local_port < UDP_LAST_PORT) {
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;  /* IPv4 UDP server */
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(local_port);

    int r = ::bind(fd_, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if (r == 0) {
      // bind successful
      port_ = local_port;
      return true;
    }

    // failed to bind, try again
    printf("bind, port %d: %s\n", local_port, strerror(errno));
    ++local_port;
  }

  // permanently failed to bind
  return false;
}

} // namespace lib
} // namespace freud
