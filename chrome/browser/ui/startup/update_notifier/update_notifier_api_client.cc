#include "update_notifier_api_client.h"

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
  // todo: testing with supabase api, need to implement our own api
  GURL api_url{base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                             "yqeigqpelhcympgbedvj.supabase.co", "/",
                             "rest/v1/updater"})};
  DCHECK(api_url.is_valid()) << "Invalid API Url: " << api_url.spec();

  base::flat_map<std::string, std::string> headers;
  headers.emplace("apikey",
                  "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
                  "eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InlxZWlncXBlbGhjeW1wZ2JlZHZq"
                  "Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NDg1Njk2NDEsImV4cCI6MjA2NDE0"
                  "NTY0MX0.tbQQBBUyYjHlqkYKuvK7ETPI0q6suq0KXLCTdNWJmYs");

  // todo: to provide version number to the api
  // const std::string payload = "{\"version\":\"" + data + "\"}";

  web_request_helper_.Request(kHttpMethod, api_url, {}, kContentType,
                              std::move(callback), headers, {});
}
