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

#include <string>

namespace freud {
namespace lib {

class Configurator {
 public:
  // this constructor initializes the configuration using default values
  Configurator();
  // read config from argc/argv, or use defaults when not available
  Configurator(const int argc, const char *argv[]);

  ~Configurator() = default;

  const std::string& get_database_directory() const;
  const std::string& get_portfile_filename() const;
  const std::string& get_elastic_search_url() const;
  bool get_cache_packets_in_db() const;
  bool get_send_packets_to_es() const;

 private:
  std::string database_directory_;
  std::string portfile_filename_;
  std::string elastic_search_url_;

  bool cache_packets_in_db_;
  bool send_packets_to_es_;

  void read_config_from_file(FILE *fp);
  static bool parse_string(const char *buf, std::string *output);
  static bool parse_bool(const char *buf, bool *output);
};

} // namespace lib
} // namespace freud
