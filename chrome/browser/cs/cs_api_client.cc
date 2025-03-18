#include "cs_api_client.h"

namespace {

constexpr char kHttpMethod[] = "POST";

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
}  // namespace

CSApiClient::CSApiClient(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : web_request_helper_(GetNetworkTrafficAnnotationTag(),
                          std::move(url_loader_factory)) {}

CSApiClient::~CSApiClient() = default;

void CSApiClient::Post(std::string data, ResultCallback callback) {
  GURL api_url{base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                             "analytics.ahrefs.com", "/", "api/event"})};
  DCHECK(api_url.is_valid()) << "Invalid API Url: " << api_url.spec();

  DVLOG(0) << "|>> Sending url : " << data;

  base::Value::Dict dict;
  dict.Set("n", "pageview");
  dict.Set("u", data);
  dict.Set("k", "RrZUA7XvGI6R2DvyqbnEOw");
  std::string json_payload;
  base::JSONWriter::Write(dict, &json_payload);

  base::flat_map<std::string, std::string> headers;
  web_request_helper_.Request(kHttpMethod, api_url, json_payload,
                              "application/json", std::move(callback), headers,
                              {});
}
