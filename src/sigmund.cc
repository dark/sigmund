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

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <condition_variable>
#include <mutex>
#include "version.h"
#include "lib/configurator.h"
#include "lib/db_interface.h"
#include "lib/es_interface.h"
#include "lib/threaded_udp_srv.h"

std::mutex signal_mutex;
std::condition_variable signal_cv;
int last_signal = 0;

void signal_handler(const int signum) {
  if (signum != SIGTERM && signum != SIGINT)
    // treat only these two signals
    return;

  {
    std::lock_guard<std::mutex> lock_guard(signal_mutex);
    // modify shared state
    last_signal = signum;
  }

  // notify
  signal_cv.notify_one();
}

bool setup_signal_handler() {
  struct sigaction action;
  bzero(&action, sizeof(action));
  action.sa_handler = signal_handler;
  if (sigaction(SIGTERM, &action, NULL) < 0) {
    fprintf(stderr, "ERROR: sigaction(SIGTERM) failed: %s\n", strerror(errno));
    return false;
  }
  if (sigaction(SIGINT, &action, NULL) < 0) {
    fprintf(stderr, "ERROR: sigaction(SIGINT) failed: %s\n", strerror(errno));
    return false;
  }

  return true;
}

void create_portfile(const freud::lib::Configurator &config, const uint16_t port) {
  FILE* fp = fopen(config.get_portfile_filename().c_str(), "w");
  if (!fp) {
    fprintf(stderr, "ERROR: fopen portfile %s: %s\n", config.get_portfile_filename().c_str(), strerror(errno));
    return;
  }
  fprintf(fp, "%d\n", port);
  fclose(fp);

  fprintf(stderr, "INFO: portfile %s created\n", config.get_portfile_filename().c_str());
}

void delete_portfile(const freud::lib::Configurator &config) {
  if (unlink(config.get_portfile_filename().c_str()) < 0) {
    fprintf(stderr, "ERROR: unlink portfile %s: %s\n", config.get_portfile_filename().c_str(), strerror(errno));
    return;
  }
  fprintf(stderr, "INFO: portfile %s deleted\n", config.get_portfile_filename().c_str());
}

int main (void) {
  fprintf(stderr, "INFO: Sigmund %s starting\n", freud::version::g_GIT_DESCRIPTION);

  if (!setup_signal_handler()) {
    fprintf(stderr, "ERROR: failed to setup signal handler\n");
    return 1;
  }

  // read configuration
  freud::lib::Configurator config;

  // setup modules
  freud::lib::DBInterface db(config);
  if (!db.init()) {
    fprintf(stderr, "ERROR: could not init db\n");
    return 1;
  }

  freud::lib::ElasticSearchInterface es(config);
  if (!es.init()) {
    fprintf(stderr, "ERROR: could not init ElasticSearch interface\n");
    return 1;
  }

  freud::lib::Dispatcher dispatcher(config, &db, &es);

  freud::lib::ThreadedUDPServer udp(&dispatcher);
  uint16_t port = udp.start_listening();
  fprintf(stderr, "INFO: UDP server listening on port: %d\n", port);

  create_portfile(config, port);

  // the big waiting loop
  while (true) {
    std::unique_lock<std::mutex> lock(signal_mutex);
    signal_cv.wait(lock, []{return last_signal != 0;});

    if (last_signal == SIGTERM || last_signal == SIGINT)
      // if interruption signal is received, break out of the loop to
      // initiate a shutdown
      break;

    // reset value of last signal received
    last_signal = 0;
  }

  delete_portfile(config);

  // stop all modules
  fprintf(stderr, "INFO: requesting UDP server shutdown\n");
  udp.stop_listening();

  dispatcher.stop_and_wait();

  db.fini();

  return 0;
}
