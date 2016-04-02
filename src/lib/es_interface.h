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

#include <stdint.h>
#include <string>
#include <curl/curl.h>
#include "lib/freud-data.pb.h"

namespace freud {
namespace lib {

class ElasticSearchInterface {
 public:
  explicit ElasticSearchInterface(const std::string &address);
  ~ElasticSearchInterface() = default;

  bool init();

  bool post_packet(const std::string &pkt);

  static size_t curl_null_cb(void *buffer, size_t size, size_t nmemb, void *userp);

 private:
  std::string address_;

  std::string pkt_post_url_;

  CURL *pkt_post_;
  char pkt_post_errbuf_[CURL_ERROR_SIZE];
  freudpb::TrackedInstance pkt_post_pb_;

  std::string pb2json(const freudpb::TrackedInstance &pb);
  void append_kv_int32(std::string *s, const std::string &k, const int32_t v);
  void append_kv_uint32(std::string *s, const std::string &k, const uint32_t v);
  void append_kv_uint64(std::string *s, const std::string &k, const uint64_t v);
  void append_kv_string(std::string *s, const std::string &k, const std::string &v);
  void append_kv_list(std::string *s, const ::google::protobuf::RepeatedPtrField<freudpb::KeyValue> &list);
};

} // namespace lib
} // namespace freud
