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

#include "lib/configurator.h"

#include <stdio.h>
#include <string.h>

namespace freud {
namespace lib {

Configurator::Configurator() {
  database_directory_ = "/var/lib/sigmund/";
  portfile_filename_ = "/run/sigmund/portfile";
  elastic_search_url_ = "http://localhost:9200/";
  cache_packets_in_db_ = false;
  send_packets_to_es_ = true;
}

Configurator::Configurator(const int argc, const char *argv[])
    // start with defaults
    : Configurator() {
  // for now, consider argv[1] a config file
  if (argc != 2)
    // nothing to read
    return;

  fprintf(stderr, "INFO: will try and read config from %s\n", argv[1]);
  FILE *fp = fopen(argv[1], "r");
  if (!fp) {
    fprintf(stderr, "ERROR: failed to open %s: %s\n", argv[1], strerror(errno));
    return;
  }

  read_config_from_file(fp);

  fclose(fp);
}

const std::string& Configurator::get_database_directory() const {
  return database_directory_;
}

const std::string& Configurator::get_portfile_filename() const {
  return portfile_filename_;
}

const std::string& Configurator::get_elastic_search_url() const {
  return elastic_search_url_;
}

bool Configurator::get_cache_packets_in_db() const {
  return cache_packets_in_db_;
}

bool Configurator::get_send_packets_to_es() const {
  return send_packets_to_es_;
}

void Configurator::read_config_from_file(FILE *fp) {
  char *buf = NULL;
  size_t buflen = 0;
  ssize_t retval;

  while (true) {
    retval = getline(&buf, &buflen, fp);
    if (retval < 0)
      // error, or EOF reached
      break;

    // remove trailing newline, if any
    if (buf[retval - 1] == '\n')
      buf[retval - 1] = '\0';

    // all lines must follow an exact syntax
    if (strncmp(buf, "db_dir=", strlen("db_dir=")) == 0) {
      if (!parse_string(buf + strlen("db_dir="), &database_directory_))
        fprintf(stderr, "WARNING: failed to parse config line '%s'\n", buf);
      else
        fprintf(stderr, "NOTICE: using database directory '%s'\n", database_directory_.c_str());
    } else if (strncmp(buf, "portfile=", strlen("portfile=")) == 0) {
      if (!parse_string(buf + strlen("portfile="), &portfile_filename_))
        fprintf(stderr, "WARNING: failed to parse config line '%s'\n", buf);
      else
        fprintf(stderr, "NOTICE: using portfile '%s'\n", portfile_filename_.c_str());
    } else if (strncmp(buf, "es_url=", strlen("es_url=")) == 0) {
      if (!parse_string(buf + strlen("es_url="), &elastic_search_url_))
        fprintf(stderr, "WARNING: failed to parse config line '%s'\n", buf);
      else
        fprintf(stderr, "NOTICE: using Elastic Search URL '%s'\n", elastic_search_url_.c_str());
    } else if (strncmp(buf, "send_to_db=", strlen("send_to_db=")) == 0) {
      if (!parse_bool(buf + strlen("send_to_db="), &cache_packets_in_db_))
        fprintf(stderr, "WARNING: failed to parse config line '%s'\n", buf);
      else
        fprintf(stderr, "NOTICE:%s sending packets to DB\n", cache_packets_in_db_ ? "" : " NOT");
    } else if (strncmp(buf, "send_to_es=", strlen("send_to_es=")) == 0) {
      if (!parse_bool(buf + strlen("send_to_es="), &send_packets_to_es_))
        fprintf(stderr, "WARNING: failed to parse config line '%s'\n", buf);
      else
        fprintf(stderr, "NOTICE:%s sending packets to ES\n", send_packets_to_es_ ? "" : " NOT");
    }

  } // while (true)

  free(buf);
}

bool Configurator::parse_string(const char *buf, std::string *output) {
  if (buf[0] == '\0')
    // empty string
    return false;

  *output = buf;
  return true;
}

bool Configurator::parse_bool(const char *buf, bool *output) {
  if (strlen(buf) == strlen("true") && strcmp(buf, "true") == 0) {
    *output = true;
    return true;
  }

  if (strlen(buf) == strlen("false") && strcmp(buf, "false") == 0) {
    *output = false;
    return true;
  }

  // parse error
  return false;
}

} // namespace lib
} // namespace freud
