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
#include <sqlite3.h>
#include "lib/configurator.h"

namespace freud {
namespace lib {

class DBInterface {
 public:
  explicit DBInterface(const Configurator &config);
  ~DBInterface();

  bool init();
  void fini();

  bool cache_packet(const std::string &s);

 private:
  std::string db_directory_;
  std::string db_filename_;
  bool fini_called_;
  sqlite3 *db_handle_;

  // prepared statements
  sqlite3_stmt *insert_pkt_cache_;

  void close_handle();
};

} // namespace lib
} // namespace freud
