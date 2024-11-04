#include "completion_api_client.h"


#include <base/containers/flat_map.h>

#include <optional>
#include <string_view>
#include <utility>

#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/values.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/simple_url_loader.h"
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

    std::string CreateJSONRequestBody(const std::vector<std::string>& prompt) {
        base::Value::Dict dict;

        //  base::Value::List stop_sequences;
        //  stop_sequences.Append("\nUser:");
        //  stop_sequences.Append("\nAssistant:");

        dict.Set("stream", true);
        dict.Set("max_tokens", 1280);
        dict.Set("top_p", 0.7);
        dict.Set("temperature", 0.6);
        dict.Set("model", "Mixtral-8x7B-Instruct-v0.1");
        //  dict.Set("stop_sequences", std::move(stop_sequences));

        base::Value::List prompt_messages;
        for (const auto& item : prompt) {
            base::Value::Dict message;
            message.Set("content", std::move(item));
            message.Set("role", "user");
            prompt_messages.Append(std::move(message));
        }
        dict.Set("messages", std::move(prompt_messages));

        std::string json;
        base::JSONWriter::Write(dict, &json);
        return json;
    }

}  // namespace

CompletionApiClient::CompletionApiClient(
        scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
        : api_request_helper_(GetNetworkTrafficAnnotationTag(),
                              std::move(url_loader_factory)) {}

CompletionApiClient::~CompletionApiClient() = default;

void CompletionApiClient::QueryPrompt(
        const std::string& prompt,
        GenerationCompletedCallback data_completed_callback,
        GenerationDataCallback
        data_received_callback /* = base::NullCallback() */) {

    GURL api_url{base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                               "api.yep.com", "/", "v1/chat/completions"})};
    DCHECK(api_url.is_valid()) << "Invalid API Url: " << api_url.spec();

    base::flat_map<std::string, std::string> headers;
    headers.emplace("Accept", "text/event-stream");

    auto on_received = base::BindRepeating(
            &CompletionApiClient::OnQueryDataReceived, weak_ptr_factory_.GetWeakPtr(),
            std::move(data_received_callback));
    auto on_complete = base::BindOnce(&CompletionApiClient::OnQueryCompleted,
                                      weak_ptr_factory_.GetWeakPtr(),
                                      std::move(data_completed_callback));

    std::vector<std::string> prompts = {prompt};
    const std::string request_body = CreateJSONRequestBody(prompts);

    api_request_helper_.RequestSSE(kHttpMethod, api_url, request_body,
                                   "application/json", std::move(on_received),
                                   std::move(on_complete), headers, {});
}

void CompletionApiClient::ClearAllQueries() {
    api_request_helper_.CancelAll();
}

void CompletionApiClient::OnQueryDataReceived(
        GenerationDataCallback callback,
        base::expected<base::Value, std::string> result) {
    if (!result.has_value() || !result->is_dict()) {
        return;
    }
    const base::Value::List* list = result->GetDict().FindList("choices");
    if (list) {
        for (const auto& item : *list) {
            if (item.is_dict()) {
               const base::Value::Dict* delta = item.GetDict().FindDict("delta");
               if (delta) {
                   const std::string* content = delta->FindString("content");
                   if (content) {
                       LOG(INFO) << "delta content: " << *content;
                   }
               }
            }
        }
    }

    // This client only supports completion events
    const std::string* completion = result->GetDict().FindString("choices");
    if (completion) {
        callback.Run(std::move(*completion));
    }
}

void CompletionApiClient::OnQueryCompleted(
        GenerationCompletedCallback callback,
        APIRequestResult result) {
    const bool success = result.Is2XXResponseCode();
    // Handle successful request
    if (success) {
        std::string completion = "";
        // We're checking for a value body in case for non-streaming API results.
        if (result.value_body().is_dict()) {
            const std::string* value =
                    result.value_body().GetDict().FindString("choices");
            if (value) {
                // Trimming necessary for Llama 2 which prepends responses with a " ".
                completion = base::TrimWhitespaceASCII(*value, base::TRIM_ALL);
            }
        }

       // std::move(callback).Run(base::ok(std::move(completion)));
        return;
    }


    // Handle error
    chat::mojom::APIError error;

    if (net::HTTP_TOO_MANY_REQUESTS == result.response_code()) {
        error = chat::mojom::APIError::RateLimitReached;
    } else if (net::HTTP_REQUEST_ENTITY_TOO_LARGE == result.response_code()) {
        error = chat::mojom::APIError::ContextLimitReached;
    } else {
        error = chat::mojom::APIError::ConnectionError;
    }

    auto _ = error;

  //  std::move(callback).Run(base::unexpected(std::move(error)));
}

