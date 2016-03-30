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

#include "lib/db_interface.h"

namespace freud {
namespace lib {

DBInterface::DBInterface(const std::string &db_directory)
    : db_directory_(db_directory), fini_called_(false), db_handle_(NULL) {
  db_filename_ = db_directory_ + "/sqlite.db";
}

DBInterface::~DBInterface() {
  fini();
}

bool DBInterface::init() {
  int res = sqlite3_open(db_filename_.c_str(), &db_handle_);
  if (res != SQLITE_OK) {
    fprintf(stderr, "ERROR: sqlite3_open %s: %s\n", db_filename_.c_str(), sqlite3_errmsg(db_handle_));
    if (sqlite3_close(db_handle_) != SQLITE_OK)
      fprintf(stderr, "ERROR: sqlite3_close %s: %s\n", db_filename_.c_str(), sqlite3_errmsg(db_handle_));
    db_handle_ = NULL;
    return false;
  }

  fprintf(stderr, "INFO: DB init'd at %s\n", db_filename_.c_str());
  return true;
}

void DBInterface::fini() {
  if (fini_called_)
    return;
  fini_called_ = true;

  if (sqlite3_close(db_handle_) != SQLITE_OK)
    fprintf(stderr, "ERROR: sqlite3_close %s: %s\n", db_filename_.c_str(), sqlite3_errmsg(db_handle_));
  db_handle_ = NULL;
  fprintf(stderr, "INFO: DB closed at %s\n", db_filename_.c_str());
}

} // namespace lib
} // namespace freud
