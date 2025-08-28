// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "update_notifier_api_client.h"
#include "base/version_info/version_info.h"
#include "chrome/browser/buildflags.h"

namespace {

constexpr char kHttpMethod[] = "GET";
constexpr char kContentType[] = "application/json";

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("update_notifier", R"(
      semantics {
        sender: "Browser"
        description:
          "This is used to check if there is any update available for the users."
        trigger:
          "Triggered by user opening the browser."
        data:
          "Generated data."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

}  // namespace

UpdateNotifierApiClient::UpdateNotifierApiClient(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : web_request_helper_(GetNetworkTrafficAnnotationTag(),
                          std::move(url_loader_factory)) {}

UpdateNotifierApiClient::~UpdateNotifierApiClient() = default;

void UpdateNotifierApiClient::Post(std::string data, ResultCallback callback) {
  GURL api_url(BUILDFLAG(TAKTAK_UPDATE_CHECK_API_URL) + std::string("/") + std::string(version_info::GetVersionNumber()));
  DCHECK(api_url.is_valid()) << "Invalid API Url: " << api_url.spec();

  base::flat_map<std::string, std::string> headers;
  headers.emplace("x-api-key", BUILDFLAG(TAKTAK_UPDATE_CHECK_API_KEY));

  web_request_helper_.Request(kHttpMethod, api_url, {}, kContentType,
                              std::move(callback), headers, {});
}
