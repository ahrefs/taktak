// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef CHROMIUM_CS_API_CLIENT_H
#define CHROMIUM_CS_API_CLIENT_H

#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/json/json_writer.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "components/web_request_helper/web_request_helper.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/backoff_entry.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

using web_request_helper::WebRequestResult;

class CSApiClient {
 public:
  using ResultCallback = base::OnceCallback<void(WebRequestResult)>;

  CSApiClient(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  CSApiClient(const CSApiClient&) = delete;
  CSApiClient& operator=(const CSApiClient&) = delete;
  ~CSApiClient();

  void Post(std::string data, ResultCallback callback);

 private:
  web_request_helper::WebRequestHelper web_request_helper_;
  base::WeakPtrFactory<CSApiClient> weak_ptr_factory_{this};
};
#endif  // CHROMIUM_CS_API_CLIENT_H
