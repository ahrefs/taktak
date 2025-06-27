// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "cs_handler.h"

#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

namespace {
std::unordered_map<std::string, std::string> parseQuery(
    const std::string& query) {
  std::unordered_map<std::string, std::string> params;
  std::istringstream stream(query);
  std::string pair;

  while (std::getline(stream, pair, '&')) {
    size_t equals_pos = pair.find('=');
    if (equals_pos != std::string::npos) {
      std::string key = pair.substr(0, equals_pos);
      std::string value = pair.substr(equals_pos + 1);
      params[key] = value;
    }
  }

  return params;
}
}  // namespace

namespace cs_handler {

CSHandler::CSHandler(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  api_client_ = std::make_unique<CSApiClient>(std::move(url_loader_factory));
}

CSHandler::~CSHandler() = default;

void CSHandler::Handle(const GURL& url) {
  if (url.SchemeIsHTTPOrHTTPS()) {
    std::string url_to_submit;
    GURL::Replacements remove_query_and_ref;
    remove_query_and_ref.ClearQuery();
    remove_query_and_ref.ClearRef();
    url_to_submit = url.ReplaceComponents(remove_query_and_ref).spec();

    const std::string query = url.query();
    const std::string query_with_qm = std::string("?") + query;

    constexpr std::size_t npos = std::string::npos;
    const bool has_q_param = (query_with_qm.find("?q=") != npos) ||
                             (query_with_qm.find("&q=") != npos);

    const bool has_p_param = (query_with_qm.find("?p=") != npos) ||
                             (query_with_qm.find("&p=") != npos);

    const bool has_wd_or_word_param =
        (query_with_qm.find("?wd=") != npos) ||
        (query_with_qm.find("&wd=") != npos) ||
        (query_with_qm.find("?word=") != npos) ||
        (query_with_qm.find("&word=") != npos);

    const bool has_query_param = (query_with_qm.find("?query=") != npos) ||
                                 (query_with_qm.find("&query=") != npos);

    const bool has_text_param = (query_with_qm.find("?text=") != npos) ||
                                (query_with_qm.find("&text=") != npos);

    const bool has_mt_param = (query_with_qm.find("?MT=") != npos) ||
                              (query_with_qm.find("&MT=") != npos);

    const bool has_s_param = (query_with_qm.find("?s=") != npos) ||
                             (query_with_qm.find("&s=") != npos);

    const bool has_search_term_param =
        has_q_param || has_p_param || has_wd_or_word_param ||
        has_query_param || has_text_param || has_mt_param || has_s_param;

    if (has_search_term_param) {
      auto params = parseQuery(query);
      if (has_q_param) {
        url_to_submit += "?q=" + params["q"];
      }
      if (has_p_param) {
        url_to_submit += "?p=" + params["p"];
      }
      if (has_wd_or_word_param) {
        url_to_submit +=
            "?wd=" +
            (params["wd"].length() > 0 ? params["wd"] : params["word"]);
      }
      if (has_query_param) {
        url_to_submit += "?query=" + params["query"];
      }
      if (has_text_param) {
        url_to_submit += "?text=" + params["text"];
      }
      if (has_mt_param) {
        url_to_submit += "?MT=" + params["MT"];
      }
      if (has_s_param) {
        url_to_submit += "?s=" + params["s"];
      }
    }

    api_client_->Post(
        url_to_submit, base::BindOnce([](WebRequestResult result) {
          VLOG(0) << " |>> CS post response code : "
                   << result.response_code();
          VLOG(0) << " |>> CS post error code : " << result.error_code();
        }));
  }
}
}  // namespace cs_handler
