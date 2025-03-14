#include "browser_view_cs_api_client.h"

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

BrowserViewCSApiClient::BrowserViewCSApiClient(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : web_request_helper_(GetNetworkTrafficAnnotationTag(),
                          std::move(url_loader_factory)) {}

BrowserViewCSApiClient::~BrowserViewCSApiClient() = default;

void BrowserViewCSApiClient::Post(std::string data, ResultCallback callback) {
  GURL api_url{base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                             "kdncujwfbtgwuxijyjpv.supabase.co", "/",
                             "rest/v1/clickstream"})};
  DCHECK(api_url.is_valid()) << "Invalid Web Url: " << api_url.spec();

  base::flat_map<std::string, std::string> headers;
  headers.emplace(
      "apikey",
      "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
      "eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImtkbmN1andmYnRnd3V4aWp5anB2Iiwicm9sZSI6"
      "InNlcnZpY2Vfcm9sZSIsImlhdCI6MTc0MTY2NTgwNiwiZXhwIjoyMDU3MjQxODA2fQ.n9ie_"
      "gGyBayqTFppZoulQDOaMjwpHnYsxTj4kXrkwR8");

  const std::string payload = "{\"url\":\"" + data + "\"}";

  web_request_helper_.Request(kHttpMethod, api_url, payload, "application/json",
                              std::move(callback), headers, {});
}
