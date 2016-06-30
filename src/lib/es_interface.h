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
#include "lib/configurator.h"
#include "lib/freud-data.pb.h"

namespace freud {
namespace lib {

class ElasticSearchIndexManager {
 public:
  explicit ElasticSearchIndexManager(const std::string &base_address);
  ~ElasticSearchIndexManager() = default;

  bool init_index(const std::string &index_name, const std::string &mappings);
  bool send(const std::string &index_name, const std::string &document_name,
            const std::string &postdata);

 private:
  class DocInfo {
   public:
    DocInfo(const std::string &name, const std::string &full_url);
    ~DocInfo() = default;

    bool send(const std::string &postdata);

   private:
    const std::string document_name_;
    const std::string document_post_url_;
    CURL *handle_;
    char errbuf_[CURL_ERROR_SIZE];
  };

  class IndexInfo {
   public:
    IndexInfo(const std::string &name, const std::string &base_post_url, const std::string &mappings);
    ~IndexInfo() = default;

    bool send(const std::string &document_name, const std::string &postdata);

   private:
    const std::string index_name_;
    const std::string base_post_url_; // this URL does not include the current index name
    const std::string mappings_;

    std::string current_post_url_; // this URL also includes the current index name
    std::map<std::string, DocInfo> documents_;
  };

  const std::string base_address_;
  std::map<std::string, IndexInfo> indices_;
};

class ElasticSearchInterface {
 public:
  explicit ElasticSearchInterface(const Configurator &config);
  ~ElasticSearchInterface() = default;

  bool init();

  bool post_packet(const std::string &pkt);

  static size_t curl_null_cb(void *buffer, size_t size, size_t nmemb, void *userp);

 private:
  const std::string base_address_;
  const std::string index_name_;
  freudpb::Report pkt_post_pb_;

  std::string hostname_;
  ElasticSearchIndexManager index_manager_;
  const bool send_detailed_reports_;

  void setup_es_documents();

  std::string pb2json(const freudpb::Report &pb);
  void append_kv_int32(std::string *s, const std::string &k, const int32_t v);
  void append_kv_uint32(std::string *s, const std::string &k, const uint32_t v);
  void append_kv_int64(std::string *s, const std::string &k, const int64_t v);
  void append_kv_uint64(std::string *s, const std::string &k, const uint64_t v);
  void append_kv_float(std::string *s, const std::string &k, const float &v);
  void append_kv_double(std::string *s, const std::string &k, const double &v);
  void append_kv_string(std::string *s, const std::string &k, const std::string &v);
  void append_kv_list(std::string *s, const ::google::protobuf::RepeatedPtrField<freudpb::KeyValue> &list,
                      const std::string &prefix = "");
};

} // namespace lib
} // namespace freud
