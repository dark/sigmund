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

#include "lib/es_interface.h"

namespace freud {
namespace lib {

ElasticSearchInterface::ElasticSearchInterface(const std::string &address)
    : address_(address), pkt_post_(NULL) {
  pkt_post_url_ = address + "analyst/report/";
}

bool ElasticSearchInterface::init() {
  // init all handles
  pkt_post_ = curl_easy_init();
  curl_easy_setopt(pkt_post_, CURLOPT_URL, pkt_post_url_.c_str());
  curl_easy_setopt(pkt_post_, CURLOPT_POST, 1);
  curl_easy_setopt(pkt_post_, CURLOPT_ERRORBUFFER, pkt_post_errbuf_);
  // suppress all data output with a null callback
  curl_easy_setopt(pkt_post_, CURLOPT_WRITEFUNCTION, curl_null_cb);

  // DEBUG ONLY
  //curl_easy_setopt(pkt_post_, CURLOPT_VERBOSE, 1);

  return true;
}

bool ElasticSearchInterface::post_packet(const std::string &s) {
  if (!pkt_post_pb_.ParseFromString(s)) {
    // parse error
    fprintf(stderr, "ERROR: parse pb message failed\n");
    return false;
  }

  std::string postdata = pb2json(pkt_post_pb_);

  curl_easy_setopt(pkt_post_, CURLOPT_POSTFIELDS, postdata.c_str());
  curl_easy_setopt(pkt_post_, CURLOPT_POSTFIELDSIZE, postdata.size());
  CURLcode res = curl_easy_perform(pkt_post_);
  if (res != CURLE_OK) {
    fprintf(stderr, "ERROR: curl perform failed %d(%s): %s\n",
            res, curl_easy_strerror(res),
            // errbuf might not have been populated
            pkt_post_errbuf_[0] ? pkt_post_errbuf_ : "");
    return false;
  }

  return true;
}

size_t ElasticSearchInterface::curl_null_cb(void * /*buffer*/, size_t size, size_t nmemb, void * /*userp*/) {
  return size * nmemb;
}

std::string ElasticSearchInterface::pb2json(const freudpb::TrackedInstance &pb) {
  std::string result = "{ ";
  append_kv_int32(&result, "pid", pb.pid());
  result += ", ";
  append_kv_string(&result, "procname", pb.procname());
  result += ", ";
  append_kv_uint64(&result, "time", pb.usec_ts());
  result += ", ";
  append_kv_string(&result, "module", pb.module_name());
  result += ", ";
  append_kv_uint64(&result, "instance", pb.instance_id());
  result += ", ";

  // append array of traces
  result += "\"trace\": [";
  {
    bool first = true;
    for (const uint64_t &t : pb.trace()) {
      if (first)
        result += " ";
      else
        result += ", ";
      result += std::to_string(t);
      first = false;
    }
  }
  result += "], ";

  // append generic info
  result += "\"generic_info\": {";
  append_kv_list(&result, pb.generic_info());
  result += "}, ";

  // append module info
  result += "\"" + pb.module_name() +"\": {";
  append_kv_list(&result, pb.module_info());
  result += "} ";

  // append instance info, if present
  if (pb.has_instance_info()) {
    result += ", ";
    append_kv_string(&result, "instance_info", pb.instance_info());
  }

  result += " }";

  return result;
}

void ElasticSearchInterface::append_kv_int32(std::string *s, const std::string &k, const int32_t v) {
  s->append("\"");
  s->append(k);
  s->append("\": ");
  s->append(std::to_string(v));
  s->append(" ");
}

void ElasticSearchInterface::append_kv_uint32(std::string *s, const std::string &k, const uint32_t v) {
  s->append("\"");
  s->append(k);
  s->append("\": ");
  s->append(std::to_string(v));
  s->append(" ");
}

void ElasticSearchInterface::append_kv_uint64(std::string *s, const std::string &k, const uint64_t v) {
  s->append("\"");
  s->append(k);
  s->append("\": ");
  s->append(std::to_string(v));
  s->append(" ");
}

void ElasticSearchInterface::append_kv_string(std::string *s, const std::string &k, const std::string &v) {
  s->append("\"");
  s->append(k);
  s->append("\": \"");
  s->append(v);
  s->append("\" ");
}

void ElasticSearchInterface::append_kv_list(std::string *s,
                                            const ::google::protobuf::RepeatedPtrField<freudpb::KeyValue> &list) {
  bool first = true;
  for (const freudpb::KeyValue &kv: list) {
    switch (kv.type()) {
      case freudpb::KeyValue::INVALID:
        fprintf(stderr, "WARNING: %s invalid type for key %s\n", __FUNCTION__, kv.key().c_str());
        break;

      case freudpb::KeyValue::UINT32:
        if (kv.has_value_u32()) {
          if (!first)
            s->append(", ");
          append_kv_uint32(s, kv.key(), kv.value_u32());
          first = false;
        } else {
          fprintf(stderr, "WARNING: %s uint32 not found for key %s\n", __FUNCTION__, kv.key().c_str());
        }
        break;

      case freudpb::KeyValue::UINT64:
        if (kv.has_value_u64()) {
          if (!first)
            s->append(", ");
          append_kv_uint64(s, kv.key(), kv.value_u64());
          first = false;
        } else {
          fprintf(stderr, "WARNING: %s uint64 not found for key %s\n", __FUNCTION__, kv.key().c_str());
        }
        break;
    }
  }
}

} // namespace lib
} // namespace freud
