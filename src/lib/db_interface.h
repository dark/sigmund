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

#include <string>
#include <sqlite3.h>

namespace freud {
namespace lib {

class DBInterface {
 public:
  explicit DBInterface(const std::string &db_directory);
  ~DBInterface();

  bool init();
  void fini();

 private:
  std::string db_directory_;
  std::string db_filename_;
  bool fini_called_;
  sqlite3 *db_handle_;
};

} // namespace lib
} // namespace freud
