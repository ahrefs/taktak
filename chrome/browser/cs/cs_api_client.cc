// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "cs_api_client.h"

#include "base/uuid.h"
#include "chrome/browser/buildflags.h"
#include "components/machine_id/machine_id.h"
#include "base/hash/hash.h"
#include "base/strings/string_number_conversions.h"

namespace {

constexpr char kHttpMethod[] = "POST";
constexpr char kContentType[] = "application/json";

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("research", R"(
      semantics {
        sender: "Browser"
        description:
          "This is used for research purposes."
        trigger:
          "Triggered by user browsing the web."
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

std::string GetHashedMachineId(std::string machine_id) {
  // strip off sign bit
  uint64_t hashed_machine_id =  static_cast<uint64_t> (base::Hash(machine_id) & 0x7fffffff);
  std::string machine_id_str = base::NumberToString(hashed_machine_id);
  return machine_id_str;
}

}  // namespace

CSApiClient::CSApiClient(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : web_request_helper_(GetNetworkTrafficAnnotationTag(),
                          std::move(url_loader_factory)) {}

CSApiClient::~CSApiClient() = default;

void CSApiClient::Post(std::string data, ResultCallback callback) {
  GURL api_url{BUILDFLAG(TAKTAK_TEL_API_URL)};
  DCHECK(api_url.is_valid()) << "Invalid API Url: " << api_url.spec();

  base::Value::Dict dict;
  dict.Set("n", "pageview");
  dict.Set("u", data);
  dict.Set("k", BUILDFLAG(TAKTAK_TEL_API_KEY));
  std::string machine_id;

  if (machine_id::GetMachineId(&machine_id)) {
    dict.Set("v", GetHashedMachineId(machine_id));
  } else {
    // if machine ID is empty for some reasons, a UUID will be generated and sent.
    base::Uuid uuid = base::Uuid::GenerateRandomV4();
    machine_id = uuid.AsLowercaseString();
    dict.Set("v", machine_id);
  }

  std::string json_payload;
  base::JSONWriter::Write(dict, &json_payload);

  DVLOG(0) << "|>> Sending payload : " << json_payload;

  web_request_helper_.Request(kHttpMethod, api_url, json_payload, kContentType,
                              std::move(callback), {}, {});
}
