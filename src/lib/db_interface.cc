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
    : db_directory_(db_directory), fini_called_(false), db_handle_(NULL), insert_pkt_cache_(NULL) {
  db_filename_ = db_directory_ + "/sqlite.db";
}

DBInterface::~DBInterface() {
  fini();
}

bool DBInterface::init() {
  // open a connection
  int res = sqlite3_open(db_filename_.c_str(), &db_handle_);
  if (res != SQLITE_OK) {
    fprintf(stderr, "ERROR: sqlite3_open %s: %s\n", db_filename_.c_str(), sqlite3_errmsg(db_handle_));
    close_handle();
    return false;
  }

  fprintf(stderr, "INFO: DB init'd at %s\n", db_filename_.c_str());

  // create tables if they do not exist
  char *errmsg = NULL;
  res = sqlite3_exec(db_handle_, "CREATE TABLE IF NOT EXISTS cache (id INTEGER PRIMARY KEY, data BLOB);",
                     NULL, NULL, &errmsg);
  if (res != SQLITE_OK) {
    fprintf(stderr, "ERROR: sqlite3_exec %s: %s\n", sqlite3_errmsg(db_handle_), errmsg);
    sqlite3_free(errmsg);
    close_handle();
    return false;
  }

  fprintf(stderr, "INFO: tables init'd at %s\n", db_filename_.c_str());

  // init all prepared statements
  res = sqlite3_prepare_v2(db_handle_,
                           "INSERT INTO cache (data) VALUES (\"@pktdata\");",
                           -1,
                           &insert_pkt_cache_,
                           NULL);
  if (res != SQLITE_OK) {
    fprintf(stderr, "ERROR: prepared stmt 1 failed: %s\n", sqlite3_errmsg(db_handle_));
    close_handle();
    return false;
  }

  return true;
}

void DBInterface::fini() {
  if (fini_called_)
    return;
  fini_called_ = true;

  // finalize all prepared statements
  if (insert_pkt_cache_) {
    int res = sqlite3_finalize(insert_pkt_cache_);
    if (res != SQLITE_OK)
      // soft error
      fprintf(stderr, "WARNING: %s, finalize stmt 1 failed: %s\n", __FUNCTION__, sqlite3_errmsg(db_handle_));

    insert_pkt_cache_ = NULL;
  }

  close_handle();
  fprintf(stderr, "INFO: DB closed at %s\n", db_filename_.c_str());
}

bool DBInterface::cache_packet(const std::string &s) {
  int res = sqlite3_reset(insert_pkt_cache_);
  if (res != SQLITE_OK)
    // soft error
    fprintf(stderr, "WARNING: %s, reset failed: %s\n", __FUNCTION__, sqlite3_errmsg(db_handle_));

  // I am not going to call sqlite3_clear_bindings(), since we are
  // going to overwrite @pktdata anyway
  res = sqlite3_bind_blob(insert_pkt_cache_,
                          1,
                          s.data(), s.length(),
                          SQLITE_STATIC);
  if (res != SQLITE_OK) {
    // fatal error
    fprintf(stderr, "ERROR: %s, bind failed: %s\n", __FUNCTION__, sqlite3_errmsg(db_handle_));
    return false;
  }

  res = sqlite3_step(insert_pkt_cache_);
  if (res != SQLITE_DONE) {
    // fatal error
    fprintf(stderr, "ERROR: %s, step failed: %s\n", __FUNCTION__, sqlite3_errmsg(db_handle_));
    return false;
  }

  return true;
}

void DBInterface::close_handle() {
  if (sqlite3_close(db_handle_) != SQLITE_OK)
    fprintf(stderr, "ERROR: sqlite3_close %s: %s\n", db_filename_.c_str(), sqlite3_errmsg(db_handle_));
  db_handle_ = NULL;
}

} // namespace lib
} // namespace freud
