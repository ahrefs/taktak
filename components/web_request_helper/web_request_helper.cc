// Copyright (c) 2024 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 1.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/1.0/.

#include "web_request_helper.h"

#include <algorithm>
#include <string_view>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "base/debug/alias.h"
#include "base/debug/dump_without_crashing.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/raw_ptr.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/string_split.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "base/timer/elapsed_timer.h"
#include "base/trace_event/trace_event.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace web_request_helper {

namespace {

const unsigned int kRetriesCountOnNetworkChange = 1;

scoped_refptr<base::SequencedTaskRunner> MakeDecoderTaskRunner() {
  return base::ThreadPool::CreateSequencedTaskRunner(
      {base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
}

WebRequestResult ToWebRequestResult(
    std::unique_ptr<network::SimpleURLLoader> loader) {
  auto response_code = -1;
  auto error_code = loader->NetError();
  auto final_url = loader->GetFinalURL();
  base::flat_map<std::string, std::string> headers;
  if (loader->ResponseInfo()) {
    auto headers_list = loader->ResponseInfo()->headers;
    if (headers_list) {
      response_code = headers_list->response_code();
      DVLOG(1) << "Response code: " << response_code;
      size_t header_iter = 0;
      std::string key;
      std::string value;
      while (headers_list->EnumerateHeaderLines(&header_iter, &key, &value)) {
        key = base::ToLowerASCII(key);
        headers[key] = value;
        DVLOG(2) << "< " << key << ": " << value;
      }
    }
  }

  return WebRequestResult(response_code, base::Value(), std::move(headers),
                          error_code, final_url);
}

}  // namespace

WebRequestResult::WebRequestResult() = default;

WebRequestResult::WebRequestResult(
    int response_code,
    base::Value value_body,
    base::flat_map<std::string, std::string> headers,
    int error_code,
    GURL final_url)
    : response_code_(response_code),
      value_body_(std::move(value_body)),
      headers_(std::move(headers)),
      error_code_(error_code),
      final_url_(std::move(final_url)) {}

WebRequestResult::WebRequestResult(WebRequestResult&&) = default;

WebRequestResult& WebRequestResult::operator=(WebRequestResult&&) = default;

WebRequestResult::~WebRequestResult() = default;

bool WebRequestResult::operator==(const WebRequestResult& other) const {
  auto tied = [](auto& v) {
    return std::tie(v.response_code_, v.value_body_, v.headers_, v.error_code_,
                    v.final_url_);
  };
  return tied(*this) == tied(other);
}

bool WebRequestResult::operator!=(const WebRequestResult& other) const {
  return !(*this == other);
}

bool WebRequestResult::Is2XXResponseCode() const {
  return response_code_ >= 200 && response_code_ <= 299;
}

bool WebRequestResult::IsResponseCodeValid() const {
  return response_code_ >= 100 && response_code_ <= 599;
}

base::Value WebRequestResult::TakeBody() {
  CHECK(!body_consumed_);
  body_consumed_ = true;
  return std::move(value_body_);
}

std::string WebRequestResult::SerializeBodyToString() const {
  if (value_body_.is_none()) {
    return std::string();
  }
  std::string safe_json;
  if (!base::JSONWriter::Write(value_body_, &safe_json)) {
    VLOG(1) << "Response validation error: Encoding error";
  }

  return safe_json;
}

WebRequestHelper::WebRequestHelper(
    net::NetworkTrafficAnnotationTag annotation_tag,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : annotation_tag_(annotation_tag),
      url_loader_factory_(url_loader_factory),
      task_runner_(MakeDecoderTaskRunner()) {}

WebRequestHelper::~WebRequestHelper() = default;

WebRequestHelper::Ticket WebRequestHelper::Request(
    const std::string& method,
    const GURL& url,
    const std::string& payload,
    const std::string& payload_content_type,
    ResultCallback callback,
    const base::flat_map<std::string, std::string>& headers,
    const WebRequestOptions& request_options,
    ResponseConversionCallback conversion_callback) {
  auto iter = CreateRequestURLLoaderHandler(
      method, url, payload, payload_content_type, request_options, headers,
      std::move(callback));
  auto* handler = iter->get();

  if (request_options.max_body_size == -1u) {
    handler->url_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
        url_loader_factory_.get(),
        base::BindOnce(&WebRequestHelper::URLLoaderHandler::OnResponse,
                       handler->GetWeakPtr(), std::move(conversion_callback)));
  } else {
    handler->url_loader_->DownloadToString(
        url_loader_factory_.get(),
        base::BindOnce(&WebRequestHelper::URLLoaderHandler::OnResponse,
                       handler->GetWeakPtr(), std::move(conversion_callback)),
        request_options.max_body_size);
  }

  return iter;
}

WebRequestHelper::Ticket WebRequestHelper::RequestSSE(
    const std::string& method,
    const GURL& url,
    const std::string& payload,
    const std::string& payload_content_type,
    DataReceivedCallback data_received_callback,
    ResultCallback result_callback,
    const base::flat_map<std::string, std::string>& headers,
    const WebRequestOptions& request_options) {
  return RequestSSE(method, url, payload, payload_content_type,
                    std::move(data_received_callback),
                    std::move(result_callback), headers, request_options,
                    base::NullCallback());
}

WebRequestHelper::Ticket WebRequestHelper::RequestSSE(
    const std::string& method,
    const GURL& url,
    const std::string& payload,
    const std::string& payload_content_type,
    DataReceivedCallback data_received_callback,
    ResultCallback result_callback,
    const base::flat_map<std::string, std::string>& headers,
    const WebRequestOptions& request_options,
    ResponseStartedCallback response_started_callback) {
  auto iter = CreateRequestURLLoaderHandler(
      method, url, payload, payload_content_type, request_options, headers,
      std::move(result_callback));
  auto* handler = iter->get();

  // Set streaming data callback
  handler->data_received_callback_ = std::move(data_received_callback);

  handler->response_started_callback_ = std::move(response_started_callback);

  handler->url_loader_->DownloadAsStream(url_loader_factory_.get(), handler);
  return iter;
}

void WebRequestHelper::DeleteAndSendResult(Ticket iter,
                                           ResultCallback callback,
                                           WebRequestResult result) {
  Cancel(iter);
  std::move(callback).Run(std::move(result));
}

void WebRequestHelper::Cancel(const Ticket& ticket) {
  url_loaders_.erase(ticket);
}

void WebRequestHelper::CancelAll() {
  url_loaders_.clear();
}

WebRequestHelper::Ticket WebRequestHelper::CreateURLLoaderHandler(
    const std::string& method,
    const GURL& url,
    const std::string& payload,
    const std::string& payload_content_type,
    bool auto_retry_on_network_change,
    bool enable_cache,
    bool allow_http_error_result,
    const base::flat_map<std::string, std::string>& headers) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = url;
  request->load_flags = net::LOAD_DO_NOT_SAVE_COOKIES;
  if (!enable_cache) {
    request->load_flags =
        request->load_flags | net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE;
  }

  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  if (!method.empty()) {
    request->method = method;
  }

  DVLOG(1) << method << " " << url.spec();

  if (!headers.empty()) {
    for (auto entry : headers) {
      DVLOG(4) << "> " << entry.first << ": " << entry.second;
      request->headers.SetHeader(entry.first, entry.second);
    }
  }

  auto url_loader =
      network::SimpleURLLoader::Create(std::move(request), annotation_tag_);
  if (!payload.empty()) {
    url_loader->AttachStringForUpload(payload, payload_content_type);
  }
  url_loader->SetRetryOptions(
      kRetriesCountOnNetworkChange,
      auto_retry_on_network_change
          ? network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE
          : network::SimpleURLLoader::RetryMode::RETRY_NEVER);
  url_loader->SetAllowHttpErrorResults(allow_http_error_result);

  auto loader_wrapper_handler =
      std::make_unique<URLLoaderHandler>(this, task_runner_);
  loader_wrapper_handler->RegisterURLLoader(std::move(url_loader));

  auto iter = url_loaders_.insert(url_loaders_.begin(),
                                  std::move(loader_wrapper_handler));

  return iter;
}

WebRequestHelper::Ticket WebRequestHelper::CreateRequestURLLoaderHandler(
    const std::string& method,
    const GURL& url,
    const std::string& payload,
    const std::string& payload_content_type,
    const WebRequestOptions& request_options,
    const base::flat_map<std::string, std::string>& headers,
    ResultCallback result_callback) {
  auto iter = CreateURLLoaderHandler(
      method, url, payload, payload_content_type,
      request_options.auto_retry_on_network_change,
      request_options.enable_cache, true /* allow_http_error_result*/, headers);
  auto* handler = iter->get();

  handler->result_callback_ = base::BindOnce(
      &WebRequestHelper::DeleteAndSendResult, weak_ptr_factory_.GetWeakPtr(),
      iter, std::move(result_callback));
  if (request_options.timeout) {
    handler->url_loader_->SetTimeoutDuration(request_options.timeout.value());
  }
  return iter;
}

WebRequestHelper::URLLoaderHandler::URLLoaderHandler(
    WebRequestHelper* api_request_helper,
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : api_request_helper_(api_request_helper),
      previous_invalid_piece_of_response_chunk_(""),
      task_runner_(std::move(task_runner)) {}

WebRequestHelper::URLLoaderHandler::~URLLoaderHandler() = default;

void WebRequestHelper::URLLoaderHandler::RegisterURLLoader(
    std::unique_ptr<network::SimpleURLLoader> loader) {
  url_loader_ = std::move(loader);

  auto on_response_start =
      [](base::WeakPtr<WebRequestHelper::URLLoaderHandler> handler,
         const GURL& final_url,
         const network::mojom::URLResponseHead& response_head) {
        if (handler) {
          if (response_head.mime_type == "text/event-stream") {
            handler->is_sse_ = true;
          }
          if (handler->response_started_callback_) {
            std::move(handler->response_started_callback_)
                .Run(final_url.spec(), response_head.content_length);
          }
        }
      };

  url_loader_->SetOnResponseStartedCallback(base::BindOnce(
      std::move(on_response_start), weak_ptr_factory_.GetWeakPtr()));
}

base::WeakPtr<WebRequestHelper::URLLoaderHandler>
WebRequestHelper::URLLoaderHandler::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void WebRequestHelper::URLLoaderHandler::ParseJsonImpl(
    std::string json,
    base::OnceCallback<void(ValueOrError)> callback) {
  if (!data_decoder_) {
    VLOG(0) << "Creating DataDecoder for WebRequestHelper";
    data_decoder_ = std::make_unique<data_decoder::DataDecoder>();
  }

  data_decoder_->ParseJson(json, std::move(callback));
}

void WebRequestHelper::URLLoaderHandler::OnDataReceived(
    std::string_view string_piece,
    base::OnceClosure resume) {
  DVLOG(1) << "[[" << __func__ << "]]" << " Chunk received";
  if (is_sse_) {
    ParseSSE(string_piece);
  } else {
    DVLOG(1) << "Chunk content: \n" << string_piece;
    data_received_callback_.Run(base::Value(string_piece));
  }
  std::move(resume).Run();
}

void WebRequestHelper::URLLoaderHandler::OnComplete(bool success) {
  DCHECK(result_callback_);
  DVLOG(1) << "[[" << __func__ << "]]" << " Response completed\n";

  request_is_finished_ = true;

  // Delete now or when decoding operations are complete
  MaybeSendResult();
}

void WebRequestHelper::URLLoaderHandler::OnRetry(
    base::OnceClosure start_retry) {
  std::move(start_retry).Run();
}

void WebRequestHelper::URLLoaderHandler::OnResponse(
    ResponseConversionCallback conversion_callback,
    const std::unique_ptr<std::string> response_body) {
  DVLOG(1) << "[[" << __func__ << "]]" << " Response received\n";
  DCHECK(result_callback_);

  DCHECK_EQ(current_decoding_operation_count_, 0);
  WebRequestResult result = ToWebRequestResult(std::move(url_loader_));

  if (!response_body) {
    std::move(result_callback_).Run(std::move(result));
    return;
  }
  auto& raw_body = *response_body;
  if (conversion_callback) {
    auto converted_body = std::move(conversion_callback).Run(raw_body);
    if (!converted_body) {
      result.response_code_ = 422;
      std::move(result_callback_).Run(std::move(result));
      return;
    }
    raw_body = converted_body.value();
  }

  ParseJsonImpl(
      std::move(raw_body),
      base::BindOnce(&WebRequestHelper::URLLoaderHandler::OnParseJsonResponse,
                     GetWeakPtr(), std::move(result)));
}

void WebRequestHelper::URLLoaderHandler::OnParseJsonResponse(
    WebRequestResult result,
    ValueOrError result_value) {
  if (!result_value.has_value()) {
    DVLOG(1) << "Response validation error:" << result_value.error();
    if (result_value.error().starts_with("trailing comma")) {
      DEBUG_ALIAS_FOR_GURL(url_alias, result.final_url());
      DEBUG_ALIAS_FOR_CSTR(result_str, result_value.error().c_str(), 1024);
      base::debug::DumpWithoutCrashing();
    }
    std::move(result_callback_).Run(std::move(result));
    return;
  }
  if (!result_value.value().is_dict() && !result_value.value().is_list()) {
    DVLOG(1) << "Response validation error: Invalid top-level type";
    std::move(result_callback_).Run(std::move(result));
    return;
  }

  DVLOG(1) << "Response validation successful";
  result.value_body_ = std::move(result_value.value());
  std::move(result_callback_).Run(std::move(result));
}

void WebRequestHelper::URLLoaderHandler::MaybeSendResult() {
  DCHECK_LE(0, current_decoding_operation_count_);
  const bool decoding_is_complete = (current_decoding_operation_count_ == 0);

  DVLOG(1) << "request_is_finished_: " << request_is_finished_ << std::endl
           << "decoding_is_complete:" << decoding_is_complete;

  if (!request_is_finished_ && decoding_is_complete) {
    DVLOG(1)
        << "Did not run URLLoaderHandler completion handler, maybe still have "
        << current_decoding_operation_count_
        << " decoding operations in progress.";
  }

  // Don't wait for pending decoding operations it ain't matter
  if (request_is_finished_) {
    std::move(result_callback_).Run(ToWebRequestResult(std::move(url_loader_)));
  }
}

void WebRequestHelper::URLLoaderHandler::ParseSSE(
    std::string_view string_piece) {
  // New chunks should only be received before the request is completed
  DCHECK(!request_is_finished_);

  // We split the string into multiple chunks because there are cases where
  // multiple chunks are received in a single call.
  std::vector<std::string_view> stream_data = base::SplitStringPiece(
      string_piece, "\r\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  DVLOG(1) << "StringPiece(string_view): " << string_piece;

  // Occasionally, one of the response chunks may be divided into
  // two parts—specifically, the last chunk of one response and the
  // first chunk of the following response. We need to merge these
  // chunks to create valid JSON. The last invalid chunk will be
  // discarded, and the merged chunk will replace the first invalid
  // chunk of the subsequent response.
  static constexpr char kDataPrefix[] = "data: {";
  static constexpr char kDataSuffix[] = "}]}";

  std::vector<std::string> stream_data_copy;

  if (!stream_data.empty()) {
    bool is_first_piece_invalid = false;
    bool is_last_piece_invalid = false;
    auto first = stream_data[0];
    if (!base::StartsWith(first, kDataPrefix)) {
      is_first_piece_invalid = true;
      DVLOG(1) << "Chunk doesn't start with SSE prefix. Invalid JSON.";
      DVLOG(1) << "Invalid first chunk: " << first;
      if (!previous_invalid_piece_of_response_chunk_.empty()) {
        std::string combined_chunk =
            std::string(previous_invalid_piece_of_response_chunk_) +
            (std::string(first));
        stream_data_copy.push_back(combined_chunk);
        previous_invalid_piece_of_response_chunk_ = "";
        DVLOG(1) << "Replaced invalid chunk with valid one: " << combined_chunk;
      }
    }
    if (stream_data.size() > 1) {
      auto last = stream_data[stream_data.size() - 1];
      if (base::StartsWith(last, kDataPrefix) &&
          !base::EndsWith(last, kDataSuffix)) {
        is_last_piece_invalid = true;
        DVLOG(1) << "Chunk starts with SSE prefix but doesn't end with "
                    "SSE suffix. Invalid JSON.";
        DVLOG(1) << "Invalid last chunk: " << last;
        previous_invalid_piece_of_response_chunk_ = std::string(last);
      }
    }

    auto size = stream_data.size();
    for (size_t i = 0; i < size; i++) {
      if (is_first_piece_invalid && i == 0) {
        continue;
      }
      if (is_last_piece_invalid && i == size - 1) {
        continue;
      }
      stream_data_copy.push_back(std::string(stream_data[i]));
    }
  }

  // Keep track of number of in-progress data decoding operations
  // so that we can know if any are still in-progress when the request
  // completes.
  current_decoding_operation_count_ += stream_data_copy.size();

  for (const auto& data : stream_data_copy) {
    auto start_index = strlen(kDataPrefix) - 1;
    if (start_index >= data.size()) {
      DVLOG(1) << "Start index is out of bounds.";
      continue;
    }
    auto json = data.substr(start_index);
    auto on_json_parsed =
        [](base::WeakPtr<WebRequestHelper::URLLoaderHandler> handler,
           ValueOrError result) {
          DVLOG(2) << "Chunk parsed";
          if (!handler) {
            return;
          }
          handler->current_decoding_operation_count_--;
          DCHECK(handler->data_received_callback_);
          handler->data_received_callback_.Run(std::move(result));
          // Parsing is potentially the last operation for |URLLoaderHandler|.
          handler->MaybeSendResult();
        };

    DVLOG(1) << "Going to call ParseJsonImpl";
    ParseJsonImpl(std::string(json),
                  base::BindOnce(std::move(on_json_parsed),
                                 weak_ptr_factory_.GetWeakPtr()));
  }
}

void WebRequestHelper::SetUrlLoaderFactoryForTesting(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  url_loader_factory_ = std::move(url_loader_factory);
}

void SanitizeAndParseJson(std::string json,
                          base::OnceCallback<void(ValueOrError)> callback) {
  data_decoder::DataDecoder::ParseJsonIsolated(json, std::move(callback));
}
}  // namespace web_request_helper
