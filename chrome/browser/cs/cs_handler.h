// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef CHROMIUM_CS_HANDLER_H
#define CHROMIUM_CS_HANDLER_H

#include <memory>
#include <string>

#include "components/web_request_helper/web_request_helper.h"
#include "cs_api_client.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace cs_handler {

class CSHandler {
 public:
  CSHandler(scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  CSHandler(const CSHandler&) = delete;
  CSHandler& operator=(const CSHandler&) = delete;
  ~CSHandler();

  void HandleURL(const GURL& gurl);
  void HandleChatCustomEvent(const std::string event_name);
  void HandleLaunchingCustomEvent(
      const std::string event_name,
      base::OnceCallback<void(web_request_helper::WebRequestResult)> callback);

 private:
  std::unique_ptr<CSApiClient> api_client_;
  base::WeakPtrFactory<CSHandler> weak_ptr_factory_{this};
};

}  // namespace cs_handler

#endif  // CHROMIUM_CS_HANDLER_H
