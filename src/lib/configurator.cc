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

namespace freud {
namespace lib {

Configurator::Configurator() {
  database_directory_ = "/var/lib/sigmund/";
  portfile_filename_ = "/run/sigmund/portfile";
  elastic_search_url_ = "http://localhost:9200/";
  cache_packets_in_db_ = false;
  send_packets_to_es_ = true;
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

} // namespace lib
} // namespace freud
