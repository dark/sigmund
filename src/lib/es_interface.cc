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

ElasticSearchIndexManager::ElasticSearchIndexManager(const std::string &base_address)
    : base_address_(base_address) {
}

bool ElasticSearchIndexManager::init_index(const std::string &index_name, const std::string &mappings) {
  auto index_ptr = indices_.find(index_name);
  if (index_ptr != indices_.end()) {
    // index already present
    fprintf(stderr, "ERROR: index named [%s] exists already\n", index_name.c_str());
    return false;
  }

  indices_.insert(std::pair<std::string, IndexInfo>(index_name,
                                                    IndexInfo(index_name,
                                                              base_address_ + index_name + "/",
                                                              mappings)));
  fprintf(stderr, "INFO: created new index named [%s]\n", index_name.c_str());

  return true;
}

bool ElasticSearchIndexManager::send(const std::string &index_name, const std::string &document_name,
                                     const std::string &postdata) {
  auto index_ptr = indices_.find(index_name);
  if (index_ptr == indices_.end()) {
    // index not found
    fprintf(stderr, "ERROR: index named [%s] not found while sending\n", index_name.c_str());
    return false;
  }

  return index_ptr->second.send(document_name, postdata);
}

ElasticSearchIndexManager::IndexInfo::IndexInfo(const std::string &name, const std::string &index_url,
                                                const std::string &mappings)
    : index_name_(name), index_post_url_(index_url), mappings_(mappings) {
  char tmp_errbuf[CURL_ERROR_SIZE];
  CURL *tmp_handle = curl_easy_init();
  curl_easy_setopt(tmp_handle, CURLOPT_URL, index_post_url_.c_str());
  curl_easy_setopt(tmp_handle, CURLOPT_POST, 1);
  curl_easy_setopt(tmp_handle, CURLOPT_ERRORBUFFER, tmp_errbuf);
  curl_easy_setopt(tmp_handle, CURLOPT_WRITEFUNCTION, ElasticSearchInterface::curl_null_cb);
  curl_easy_setopt(tmp_handle, CURLOPT_POSTFIELDS, mappings.c_str());
  curl_easy_setopt(tmp_handle, CURLOPT_POSTFIELDSIZE, mappings.size());

  CURLcode res = curl_easy_perform(tmp_handle);
  if (res != CURLE_OK)
    fprintf(stderr, "WARNING: ES mapping init failed for index %s: %d(%s), %s\n",
            index_name_.c_str(),
            res, curl_easy_strerror(res),
            // errbuf might not have been populated
            tmp_errbuf[0] ? tmp_errbuf : "");

  curl_easy_cleanup(tmp_handle);
}

bool ElasticSearchIndexManager::IndexInfo::send(const std::string &document_name, const std::string &postdata) {
  // create document if not already existing
  auto doc_ptr = documents_.find(document_name);
  if (doc_ptr == documents_.end()) {
    // index not found
    auto retval = documents_.insert(std::pair<std::string, DocInfo>(document_name,
                                                                    DocInfo(document_name,
                                                                            index_post_url_ + document_name + "/")));
    doc_ptr = retval.first;

    fprintf(stderr, "INFO: created new document [%s] under index [%s]\n",
            document_name.c_str(), index_name_.c_str());
  }

  return doc_ptr->second.send(postdata);
}

ElasticSearchIndexManager::DocInfo::DocInfo(const std::string &name, const std::string &full_url)
    : document_name_(name), document_post_url_(full_url) {
  handle_ = curl_easy_init();
  curl_easy_setopt(handle_, CURLOPT_URL, document_post_url_.c_str());
  curl_easy_setopt(handle_, CURLOPT_POST, 1);
  curl_easy_setopt(handle_, CURLOPT_ERRORBUFFER, errbuf_);
  // suppress all data output with a null callback
  curl_easy_setopt(handle_, CURLOPT_WRITEFUNCTION, ElasticSearchInterface::curl_null_cb);
  // DEBUG ONLY
  //curl_easy_setopt(handle_, CURLOPT_VERBOSE, 1);
}

bool ElasticSearchIndexManager::DocInfo::send(const std::string &postdata) {
  curl_easy_setopt(handle_, CURLOPT_POSTFIELDS, postdata.c_str());
  curl_easy_setopt(handle_, CURLOPT_POSTFIELDSIZE, postdata.size());
  CURLcode res = curl_easy_perform(handle_);
  if (res != CURLE_OK) {
    fprintf(stderr, "ERROR: curl perform failed at URL[%s]: %d(%s), %s\n",
            document_post_url_.c_str(),
            res, curl_easy_strerror(res),
            // errbuf might not have been populated
            errbuf_[0] ? errbuf_ : "");
    return false;
  }

  return true;
}

ElasticSearchInterface::ElasticSearchInterface(const Configurator &config)
    : base_address_(config.get_elastic_search_url()), index_name_(config.get_elastic_search_index()),
      index_manager_(base_address_),
      send_detailed_reports_(config.fwd_detailed_reports()) {
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

  return true;
}

bool ElasticSearchInterface::post_packet(const std::string &s) {
  if (!pkt_post_pb_.ParseFromString(s)) {
    // parse error
    fprintf(stderr, "ERROR: parse pb message failed\n");
    return false;
  }

  if (pkt_post_pb_.type() == freudpb::Report::DETAILED && !send_detailed_reports_)
    // nothing to do here, we don't want to send this detailed report
    return true;

  std::string postdata = pb2json(pkt_post_pb_);

  // select URL destination based on report type
  switch (pkt_post_pb_.type()) {
    case freudpb::Report::SUMMARY:
      return index_manager_.send(index_name_, "summary-report", postdata);

    case freudpb::Report::DETAILED:
      return index_manager_.send(index_name_, "detailed-report", postdata);
  }

  return true;
}

size_t ElasticSearchInterface::curl_null_cb(void * /*buffer*/, size_t size, size_t nmemb, void * /*userp*/) {
  return size * nmemb;
}

void ElasticSearchInterface::setup_es_documents() {
  // magic JSON, update accordingly
  const std::string mappings = "{\"mappings\":{\"summary-report\":{\"properties\":{"
      "\"time\":{\"type\":\"date\",\"format\":\"epoch_millis\"},"
      "\"hostname\":{\"type\":\"string\",\"index\":\"not_analyzed\"},"
      "\"basename\":{\"type\":\"string\",\"index\":\"not_analyzed\"},"
      "\"procname\":{\"type\":\"string\",\"index\":\"not_analyzed\"},"
      "\"pgname\":{\"type\":\"string\",\"index\":\"not_analyzed\"},"
      "\"module\":{\"type\":\"string\",\"index\":\"not_analyzed\"},"
      "\"type\":{\"type\":\"string\",\"index\":\"not_analyzed\"}"
      "}},\"detailed-report\":{\"properties\":{"
      "\"time\":{\"type\":\"date\",\"format\":\"epoch_millis\"},"
      "\"hostname\":{\"type\":\"string\",\"index\":\"not_analyzed\"},"
      "\"basename\":{\"type\":\"string\",\"index\":\"not_analyzed\"},"
      "\"procname\":{\"type\":\"string\",\"index\":\"not_analyzed\"},"
      "\"pgname\":{\"type\":\"string\",\"index\":\"not_analyzed\"},"
      "\"module\":{\"type\":\"string\",\"index\":\"not_analyzed\"},"
      "\"type\":{\"type\":\"string\",\"index\":\"not_analyzed\"}"
      "}}}}";

  index_manager_.init_index(index_name_, mappings);
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
  append_kv_string(&result, "pgname", pb.pgname());
  result += ", ";
  append_kv_string(&result, "type", pb.type() == freudpb::Report::SUMMARY ? "summary" : "detailed");
  result += ", ";
  // normalize usec to msec (that's what ES expects)
  append_kv_uint64(&result, "time", pb.usec_ts() / 1000);
  result += ", ";
  append_kv_string(&result, "module", pb.module_name());
  result += ", ";
  if (pb.type() == freudpb::Report::DETAILED) {
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

  // if not a summary, append generic info here; if this a summary,
  // these info will go inside the module section instead
  if (pb.type() == freudpb::Report::DETAILED) {
    result += "\"generic_info\": {";
    append_kv_list(&result, pb.generic_info());
    result += "}, ";
  }

  // append module info
  result += "\"" + pb.module_name() +"\": {";
  append_kv_list(&result, pb.module_info());
  // if this a summary, append some metafields
  if (pb.type() == freudpb::Report::SUMMARY) {
    // need this separator only if actual module info were outputted already
    if (pb.module_info_size())
      result += ", ";

    // number of instances
    append_kv_uint64(&result, "__instances", pb.instance_id());

    // all data from the generic_info, if any, prefixed with two underscores
    if (pb.generic_info_size()) {
      result += ", ";
      append_kv_list(&result, pb.generic_info(), "__");
    }
  }
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

void ElasticSearchInterface::append_kv_int64(std::string *s, const std::string &k, const int64_t v) {
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

void ElasticSearchInterface::append_kv_float(std::string *s, const std::string &k, const float &v) {
  s->append("\"");
  s->append(k);
  s->append("\": ");
  s->append(std::to_string(v));
  s->append(" ");
}

void ElasticSearchInterface::append_kv_double(std::string *s, const std::string &k, const double &v) {
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
                                            const ::google::protobuf::RepeatedPtrField<freudpb::KeyValue> &list,
                                            const std::string &prefix) {
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
          append_kv_uint32(s, prefix + kv.key(), kv.value_u32());
          first = false;
        } else {
          fprintf(stderr, "WARNING: %s uint32 not found for key %s\n", __FUNCTION__, kv.key().c_str());
        }
        break;

      case freudpb::KeyValue::SINT32:
        if (kv.has_value_s32()) {
          if (!first)
            s->append(", ");
          append_kv_int32(s, prefix + kv.key(), kv.value_s32());
          first = false;
        } else {
          fprintf(stderr, "WARNING: %s int32 not found for key %s\n", __FUNCTION__, kv.key().c_str());
        }
        break;

      case freudpb::KeyValue::UINT64:
        if (kv.has_value_u64()) {
          if (!first)
            s->append(", ");
          append_kv_uint64(s, prefix + kv.key(), kv.value_u64());
          first = false;
        } else {
          fprintf(stderr, "WARNING: %s uint64 not found for key %s\n", __FUNCTION__, kv.key().c_str());
        }
        break;

      case freudpb::KeyValue::SINT64:
        if (kv.has_value_s64()) {
          if (!first)
            s->append(", ");
          append_kv_int64(s, prefix + kv.key(), kv.value_s64());
          first = false;
        } else {
          fprintf(stderr, "WARNING: %s int64 not found for key %s\n", __FUNCTION__, kv.key().c_str());
        }
        break;

      case freudpb::KeyValue::FLOAT:
        if (kv.has_value_float()) {
          if (!first)
            s->append(", ");
          append_kv_float(s, prefix + kv.key(), kv.value_float());
          first = false;
        } else {
          fprintf(stderr, "WARNING: %s float not found for key %s\n", __FUNCTION__, kv.key().c_str());
        }
        break;

      case freudpb::KeyValue::DOUBLE:
        if (kv.has_value_dbl()) {
          if (!first)
            s->append(", ");
          append_kv_double(s, prefix + kv.key(), kv.value_dbl());
          first = false;
        } else {
          fprintf(stderr, "WARNING: %s double not found for key %s\n", __FUNCTION__, kv.key().c_str());
        }
        break;
    }
  }
}

} // namespace lib
} // namespace freud
