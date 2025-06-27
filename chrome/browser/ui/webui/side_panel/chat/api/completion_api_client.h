// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef CHROMIUM_COMPLETION_Web_CLIENT_H
#define CHROMIUM_COMPLETION_Web_CLIENT_H

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "chrome/browser/ui/webui/side_panel/chat/chat.mojom.h"
#include "components/web_request_helper/web_request_helper.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace network {
    class SharedURLLoaderFactory;
}  // namespace network

using web_request_helper::WebRequestResult;

struct CompletionMessage {
    std::string content;
    std::string role;
};

class CompletionApiClient {
public:
 using GenerationResult =
     base::expected<std::string, chat::mojom::APIErrorType>;
 using GenerationDataCallback = base::RepeatingCallback<void(std::string)>;
 using GenerationCompletedCallback = base::OnceCallback<void(GenerationResult)>;

 CompletionApiClient(
     scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

 CompletionApiClient(const CompletionApiClient&) = delete;
 CompletionApiClient& operator=(const CompletionApiClient&) = delete;
 virtual ~CompletionApiClient();

 virtual void QueryPrompt(
     const std::vector<struct CompletionMessage>& completion_messages,
     bool enable_thinking,
     GenerationCompletedCallback data_completed_callback,
     GenerationDataCallback data_received_callback = base::NullCallback());

 // Clears all in-progress requests
 void ClearAllQueries();

private:
    void OnQueryDataReceived(GenerationDataCallback callback,
                             base::expected<base::Value, std::string> result);
    void OnQueryCompleted(GenerationCompletedCallback callback,
                          WebRequestResult result);

    web_request_helper::WebRequestHelper web_request_helper_;
    std::vector<std::string> entire_completion_result;

    base::WeakPtrFactory<CompletionApiClient> weak_ptr_factory_{this};
};

#endif  // CHROMIUM_COMPLETION_Web_CLIENT_H
