// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "completion_api_client.h"

#include <base/containers/flat_map.h>

#include <optional>
#include <string_view>
#include <utility>

#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/i18n/time_formatting.h"
#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "chrome/browser/buildflags.h"
#include "chrome/grit/generated_resources.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

namespace {

    constexpr char kHttpMethod[] = "POST";

    net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
        return net::DefineNetworkTrafficAnnotation("ai_chat", R"(
      semantics {
        sender: "AI Chat"
        description:
          "This is used to communicate with Yep Chat api."
        trigger:
          "Triggered by user sending a prompt."
        data:
          "Will generate a text that attempts to match the user gave it"
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        policy_exception_justification:
          "Not implemented."
      }
    )");
    }

    std::string CreateJSONRequestBody(const std::vector<struct CompletionMessage> &messages,
                                      bool enable_thinking) {
        base::Value::Dict dict;
        const std::string model = enable_thinking
                                      ? BUILDFLAG(TAKTAK_CHAT_DEEPSEEK_MODEL)
                                      : BUILDFLAG(TAKTAK_CHAT_MISTRAL_MODEL);
        dict.Set("max_tokens", 8'000);
        dict.Set("stream", true);
        dict.Set("top_p", 0.7);
        dict.Set("temperature", 0.6);
        dict.Set("model", model);

        base::Value::List prompt_messages;
        for (const auto &item: messages) {
            base::Value::Dict message;
            message.Set("content", std::move(item.content));
            message.Set("role", item.role);
            prompt_messages.Append(std::move(message));
        }
        dict.Set("messages", std::move(prompt_messages));

        std::string json;
        base::JSONWriter::Write(dict, &json);
        DVLOG(0) << __func__ << " |>> Request body: " << json;
        return json;
    }
}  // namespace

CompletionApiClient::CompletionApiClient(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : web_request_helper_(GetNetworkTrafficAnnotationTag(),
                          std::move(url_loader_factory)) {}

CompletionApiClient::~CompletionApiClient() = default;

void CompletionApiClient::QueryPrompt(
        const std::vector<struct CompletionMessage> &completion_messages,
        bool enable_thinking,
        GenerationCompletedCallback data_completed_callback,
        GenerationDataCallback
        data_received_callback /* = base::NullCallback() */) {
  GURL api_url{BUILDFLAG(TAKTAK_CHAT_API_URL)};
  DCHECK(api_url.is_valid()) << "Invalid Web Url: " << api_url.spec();

  base::flat_map<std::string, std::string> headers;
  headers.emplace("Accept", "text/event-stream");

  auto on_received = base::BindRepeating(
      &CompletionApiClient::OnQueryDataReceived, weak_ptr_factory_.GetWeakPtr(),
      std::move(data_received_callback));
  auto on_complete = base::BindOnce(&CompletionApiClient::OnQueryCompleted,
                                    weak_ptr_factory_.GetWeakPtr(),
                                    std::move(data_completed_callback));

  const std::string request_body =
      CreateJSONRequestBody(completion_messages, enable_thinking);

  web_request_helper_.RequestSSE(kHttpMethod, api_url, request_body,
                                 "application/json", std::move(on_received),
                                 std::move(on_complete), headers, {});
}

void CompletionApiClient::ClearAllQueries() {
    web_request_helper_.CancelAll();
    entire_completion_result.clear();
}

void CompletionApiClient::OnQueryDataReceived(
        GenerationDataCallback callback,
        base::expected<base::Value, std::string> result) {
    if (!result.has_value() || !result->is_dict()) {
        return;
    }

    const base::Value::List *list = result->GetDict().FindList("choices");
    if (list) {
        for (const auto &item: *list) {
            if (item.is_dict()) {
                const base::Value::Dict *delta = item.GetDict().FindDict("delta");
                if (delta) {
                    const std::string *content = delta->FindString("content");
                    if (content) {
                        entire_completion_result.push_back(*content);
                        callback.Run(std::move(*content));
                    }
                }
            }
        }
    }
}

void CompletionApiClient::OnQueryCompleted(GenerationCompletedCallback callback,
                                           WebRequestResult result) {
  const bool success = result.Is2XXResponseCode();

  if (success) {
    entire_completion_result.clear();
    std::move(callback).Run(base::ok(""));
    return;
  }

  // Handle error
  chat::mojom::APIErrorType error;

  if (result.value_body().is_dict()) {
    const std::string* value =
        result.value_body().GetDict().FindString("message");
    if (value) {
      DVLOG(0) << __func__ << " |>> Error message: " << *value;
    }
  }

  if (net::HTTP_TOO_MANY_REQUESTS == result.response_code()) {
    error = chat::mojom::APIErrorType::RateLimitReached;
  } else if (net::HTTP_REQUEST_ENTITY_TOO_LARGE == result.response_code()) {
    error = chat::mojom::APIErrorType::ContextLimitReached;
  } else {
    error = chat::mojom::APIErrorType::ConnectionError;
  }

  std::move(callback).Run(base::unexpected(std::move(error)));
}
