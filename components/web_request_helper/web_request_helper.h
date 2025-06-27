// Copyright (c) 2024 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 1.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/1.0/.

#ifndef CHROMIUM_WEB_REQUEST_HELPER_H
#define CHROMIUM_WEB_REQUEST_HELPER_H

#include <list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/cpp/simple_url_loader_stream_consumer.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace data_decoder {
class DataDecoder;
}

namespace web_request_helper {

class WebRequestResult {
 public:
  WebRequestResult();
  WebRequestResult(int response_code,
                   base::Value value_body,
                   base::flat_map<std::string, std::string> headers,
                   int error_code,
                   GURL final_url);
  WebRequestResult(const WebRequestResult&) = delete;
  WebRequestResult& operator=(const WebRequestResult&) = delete;
  WebRequestResult(WebRequestResult&&);
  WebRequestResult& operator=(WebRequestResult&&);
  ~WebRequestResult();

  bool operator==(const WebRequestResult& other) const;
  bool operator!=(const WebRequestResult& other) const;

  bool Is2XXResponseCode() const;
  bool IsResponseCodeValid() const;

  // HTTP response code.
  int response_code() const { return response_code_; }

  // Extract the sanitized response as base::Value.
  base::Value TakeBody();

  // Returns the sanitized response as base::Value.
  // Note: don't clone large responses, use TakeBody() instead.
  const base::Value& value_body() const { return value_body_; }

  // Serialize the sanitized response and returns it as string.
  // Note: use TakeBody()/value_body() instead where possible.
  std::string SerializeBodyToString() const;

  // HTTP response headers.
  const base::flat_map<std::string, std::string>& headers() const {
    return headers_;
  }
  // `net::Error` code
  int error_code() const { return error_code_; }
  // Actual url requested. May differ from original request url in case of
  // redirects happened.
  GURL final_url() const { return final_url_; }

 private:
  friend class WebRequestHelper;

  int response_code_ = -1;
  base::Value value_body_;
  base::flat_map<std::string, std::string> headers_;
  int error_code_ = -1;
  GURL final_url_;
  bool body_consumed_ = false;
};

struct WebRequestOptions {
  bool auto_retry_on_network_change = false;
  bool enable_cache = false;
  size_t max_body_size = -1u;
  std::optional<base::TimeDelta> timeout;
};

using ValueOrError = base::expected<base::Value, std::string>;

class WebRequestHelper {
 public:
  using DataReceivedCallback =
      base::RepeatingCallback<void(ValueOrError result)>;
  using ResultCallback = base::OnceCallback<void(WebRequestResult)>;
  using ResponseStartedCallback =
      base::OnceCallback<void(const std::string& url,
                              const int64_t content_length)>;
  using ResponseConversionCallback =
      base::OnceCallback<std::optional<std::string>(
          const std::string& raw_response)>;

  class URLLoaderHandler : public network::SimpleURLLoaderStreamConsumer {
   public:
    URLLoaderHandler(WebRequestHelper* api_request_helper,
                     scoped_refptr<base::SequencedTaskRunner> task_runner);

    ~URLLoaderHandler() override;

    URLLoaderHandler(const URLLoaderHandler&) = delete;

    URLLoaderHandler& operator=(const URLLoaderHandler&) = delete;

    void RegisterURLLoader(std::unique_ptr<network::SimpleURLLoader> loader);

    void SetResultCallback(ResultCallback result_callback);

    base::WeakPtr<URLLoaderHandler> GetWeakPtr();

   private:
    friend class WebRequestHelper;

    void ParseJsonImpl(std::string json,
                       base::OnceCallback<void(ValueOrError)> callback);

    // Run completion callback if there are no operations in progress.
    // If Cancel is needed even if url or data operations are in progress,
    // then call |WebRequestHelper::Cancel|.
    void MaybeSendResult();

    void ParseSSE(std::string_view string_piece);

    // network::SimpleURLLoaderStreamConsumer implementation:
    void OnDataReceived(std::string_view string_piece,
                        base::OnceClosure resume) override;

    void OnComplete(bool success) override;

    void OnRetry(base::OnceClosure start_retry) override;

    // This is used for one shot responses
    void OnResponse(ResponseConversionCallback conversion_callback,
                    const std::unique_ptr<std::string> response_body);

    // Decode one shot responses
    void OnParseJsonResponse(WebRequestResult result,
                             ValueOrError result_value);

    std::unique_ptr<network::SimpleURLLoader> url_loader_;
    raw_ptr<WebRequestHelper> api_request_helper_;

    DataReceivedCallback data_received_callback_;
    ResponseStartedCallback response_started_callback_;
    ResultCallback result_callback_;
    ResponseConversionCallback conversion_callback_;

    bool is_sse_ = false;

    // To ensure ordered processing of stream chunks, we create our own
    // instance of DataDecoder per request. This avoids the issue
    // of unordered chunks that can occur when calling the static function,
    // which creates a new instance of the process for each call. By using a
    // single instance of the parser, we can reuse it for consecutive calls.
    std::unique_ptr<data_decoder::DataDecoder> data_decoder_;
    // Keep track of number of in-progress data decoding operations
    // so that we can know if any are still in-progress when the request
    // completes.
    int current_decoding_operation_count_ = 0;
    bool request_is_finished_ = false;

    std::string previous_invalid_piece_of_response_chunk_;

    const scoped_refptr<base::SequencedTaskRunner> task_runner_;

    base::WeakPtrFactory<URLLoaderHandler> weak_ptr_factory_{this};
  };

  using URLLoaderHandlerList = std::list<std::unique_ptr<URLLoaderHandler>>;
  using Ticket = std::list<std::unique_ptr<URLLoaderHandler>>::iterator;

  WebRequestHelper(
      net::NetworkTrafficAnnotationTag annotation_tag,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  virtual ~WebRequestHelper();

  // Each response is expected in json format and will be validated through
  // JsonSanitizer. In cases where json contains values that are not supported
  // by the standard base/json parser it is necessary to convert such values
  // into string before validating the response. For these purposes
  // conversion_callback is added which receives raw response and can perform
  // necessary conversions.
  Ticket Request(
      const std::string& method,
      const GURL& url,
      const std::string& payload,
      const std::string& payload_content_type,
      ResultCallback callback,
      const base::flat_map<std::string, std::string>& headers = {},
      const WebRequestOptions& request_options = {},
      ResponseConversionCallback conversion_callback = base::NullCallback());

  virtual Ticket RequestSSE(
      const std::string& method,
      const GURL& url,
      const std::string& payload,
      const std::string& payload_content_type,
      DataReceivedCallback data_received_callback,
      ResultCallback result_callback,
      const base::flat_map<std::string, std::string>& headers,
      const WebRequestOptions& request_options);

  virtual Ticket RequestSSE(
      const std::string& method,
      const GURL& url,
      const std::string& payload,
      const std::string& payload_content_type,
      DataReceivedCallback data_received_callback,
      ResultCallback result_callback,
      const base::flat_map<std::string, std::string>& headers,
      const WebRequestOptions& request_options,
      ResponseStartedCallback response_started_callback);

  void Cancel(const Ticket& ticket);

  void CancelAll();

  void SetUrlLoaderFactoryForTesting(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

 private:
  WebRequestHelper(const WebRequestHelper&) = delete;

  WebRequestHelper& operator=(const WebRequestHelper&) = delete;

  WebRequestHelper::Ticket CreateURLLoaderHandler(
      const std::string& method,
      const GURL& url,
      const std::string& payload,
      const std::string& payload_content_type,
      bool auto_retry_on_network_change,
      bool enable_cache,
      bool allow_http_error_result,
      const base::flat_map<std::string, std::string>& headers);

  WebRequestHelper::Ticket CreateRequestURLLoaderHandler(
      const std::string& method,
      const GURL& url,
      const std::string& payload,
      const std::string& payload_content_type,
      const WebRequestOptions& request_options,
      const base::flat_map<std::string, std::string>& headers,
      ResultCallback result_callback);

  void DeleteAndSendResult(Ticket iter,
                           ResultCallback callback,
                           WebRequestResult result);

  net::NetworkTrafficAnnotationTag annotation_tag_;
  URLLoaderHandlerList url_loaders_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  const scoped_refptr<base::SequencedTaskRunner> task_runner_;
  base::WeakPtrFactory<WebRequestHelper> weak_ptr_factory_{this};
};

void SanitizeAndParseJson(std::string json,
                          base::OnceCallback<void(ValueOrError)> callback);

}  // namespace web_request_helper
#endif  // CHROMIUM_WEB_REQUEST_HELPER_H
