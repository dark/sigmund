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

#include <string.h> // for basename
#include <unistd.h> // for gethostname

namespace freud {
namespace lib {

ElasticSearchInterface::ElasticSearchInterface(const Configurator &config)
    : base_address_(config.get_elastic_search_url()), summary_report_handle_(NULL), detailed_report_handle_(NULL) {
  summary_report_post_url_ = base_address_ + "analyst/summary-report/";
  detailed_report_post_url_ = base_address_ + "analyst/detailed-report/";

  char buf[256];
  if (gethostname(buf, sizeof(buf)) < 0) {
    // error case
    hostname_ = "undefined";
  } else {
    // success case

    // man gethostname(2) states that POSIX does not require
    // gethostname() to raise an error if a name truncation occurred;
    // hence, to be safe, always put a \0 at the end of the buffer
    buf[sizeof(buf) - 1] = '\0';

    hostname_ = buf;
  }
}

bool ElasticSearchInterface::init() {
  // setup the documents in ES if they do not exist; failures should be ignored
  setup_es_documents();

  // init all handles
  summary_report_handle_ = curl_easy_init();
  curl_easy_setopt(summary_report_handle_, CURLOPT_URL, summary_report_post_url_.c_str());
  curl_easy_setopt(summary_report_handle_, CURLOPT_POST, 1);
  curl_easy_setopt(summary_report_handle_, CURLOPT_ERRORBUFFER, summary_report_post_errbuf_);
  // suppress all data output with a null callback
  curl_easy_setopt(summary_report_handle_, CURLOPT_WRITEFUNCTION, curl_null_cb);
  // DEBUG ONLY
  //curl_easy_setopt(summary_report_handle_, CURLOPT_VERBOSE, 1);

  detailed_report_handle_ = curl_easy_init();
  curl_easy_setopt(detailed_report_handle_, CURLOPT_URL, detailed_report_post_url_.c_str());
  curl_easy_setopt(detailed_report_handle_, CURLOPT_POST, 1);
  curl_easy_setopt(detailed_report_handle_, CURLOPT_ERRORBUFFER, detailed_report_post_errbuf_);
  // suppress all data output with a null callback
  curl_easy_setopt(detailed_report_handle_, CURLOPT_WRITEFUNCTION, curl_null_cb);
  // DEBUG ONLY
  //curl_easy_setopt(detailed_report_handle_, CURLOPT_VERBOSE, 1);

  return true;
}

bool ElasticSearchInterface::post_packet(const std::string &s) {
  if (!pkt_post_pb_.ParseFromString(s)) {
    // parse error
    fprintf(stderr, "ERROR: parse pb message failed\n");
    return false;
  }

  std::string postdata = pb2json(pkt_post_pb_);

  // select URL destination based on report type
  switch (pkt_post_pb_.type()) {
    case freudpb::Report::SUMMARY:
      {
        curl_easy_setopt(summary_report_handle_, CURLOPT_POSTFIELDS, postdata.c_str());
        curl_easy_setopt(summary_report_handle_, CURLOPT_POSTFIELDSIZE, postdata.size());
        CURLcode res = curl_easy_perform(summary_report_handle_);
        if (res != CURLE_OK) {
          fprintf(stderr, "ERROR: curl perform failed for summary report %d(%s): %s\n",
                  res, curl_easy_strerror(res),
                  // errbuf might not have been populated
                  summary_report_post_errbuf_[0] ? summary_report_post_errbuf_ : "");
          return false;
        }
      }
      break;

    case freudpb::Report::DETAILED:
      {
        curl_easy_setopt(detailed_report_handle_, CURLOPT_POSTFIELDS, postdata.c_str());
        curl_easy_setopt(detailed_report_handle_, CURLOPT_POSTFIELDSIZE, postdata.size());
        CURLcode res = curl_easy_perform(detailed_report_handle_);
        if (res != CURLE_OK) {
          fprintf(stderr, "ERROR: curl perform failed for detailed report %d(%s): %s\n",
                  res, curl_easy_strerror(res),
                  // errbuf might not have been populated
                  detailed_report_post_errbuf_[0] ? detailed_report_post_errbuf_ : "");
          return false;
        }
      }
      break;
  }

  return true;
}

size_t ElasticSearchInterface::curl_null_cb(void * /*buffer*/, size_t size, size_t nmemb, void * /*userp*/) {
  return size * nmemb;
}

void ElasticSearchInterface::setup_es_documents() {
  std::string url = base_address_ + "analyst/";

  CURL *handle = curl_easy_init();
  curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
  curl_easy_setopt(handle, CURLOPT_POST, 1);
  curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, detailed_report_post_errbuf_);
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, curl_null_cb);

  // magic JSON, update accordingly
  std::string postdata = "{\"mappings\":{\"summary-report\":{\"properties\":{\"time\":{\"type\":\"date\", \"format\":\"epoch_millis\"}}},\"detailed-report\":{\"properties\":{\"time\":{\"type\":\"date\", \"format\":\"epoch_millis\"}}}}}";
  curl_easy_setopt(handle, CURLOPT_POSTFIELDS, postdata.c_str());
  curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, postdata.size());

  CURLcode res = curl_easy_perform(handle);
  if (res != CURLE_OK)
    fprintf(stderr, "WARNING: ES document init failed %d(%s): %s\n",
            res, curl_easy_strerror(res),
            // errbuf might not have been populated
            detailed_report_post_errbuf_[0] ? detailed_report_post_errbuf_ : "");

  curl_easy_cleanup(handle);
}

std::string ElasticSearchInterface::pb2json(const freudpb::Report &pb) {
  std::string result = "{ ";
  append_kv_int32(&result, "pid", pb.pid());
  result += ", ";
  append_kv_string(&result, "hostname", hostname_);
  result += ", ";
  append_kv_string(&result, "procname", pb.procname());
  result += ", ";
  append_kv_string(&result, "basename", basename(pb.procname().c_str()));
  result += ", ";
  append_kv_string(&result, "type", pb.type() == freudpb::Report::SUMMARY ? "summary" : "detailed");
  result += ", ";
  // normalize usec to msec (that's what ES expects)
  append_kv_uint64(&result, "time", pb.usec_ts() / 1000);
  result += ", ";
  append_kv_string(&result, "module", pb.module_name());
  result += ", ";
  if (pb.type() == freudpb::Report::SUMMARY) {
    append_kv_uint64(&result, "instances", pb.instance_id());
    result += ", ";
  } else {
    append_kv_uint64(&result, "instance", pb.instance_id());
    result += ", ";
  }

  // if not a summary, append array of traces
  if (pb.type() == freudpb::Report::DETAILED) {
    result += "\"trace\": [";
    bool first = true;
    for (const uint64_t &t : pb.trace()) {
      if (first)
        result += " ";
      else
        result += ", ";
      result += std::to_string(t);
      first = false;
    }
    result += "], ";
  }

  // append generic info
  result += "\"generic_info\": {";
  append_kv_list(&result, pb.generic_info());
  result += "}, ";

  // append module info
  result += "\"" + pb.module_name() +"\": {";
  append_kv_list(&result, pb.module_info());
  result += "} ";

  // append instance info, if meaningful and present
  if (pb.type() == freudpb::Report::DETAILED && pb.has_instance_info()) {
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
