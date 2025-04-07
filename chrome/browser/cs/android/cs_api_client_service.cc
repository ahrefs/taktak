#include "cs_api_client_service.h"

#include "base/json/json_writer.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

namespace {

constexpr char kHttpMethod[] = "POST";
constexpr char kContentType[] = "application/json";
constexpr char kStagingApiKey[] = "RrZUA7XvGI6R2DvyqbnEOw";

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("cs_api_client_service", R"(
    semantics {
      sender: "CS API Client Service"
      description:
        "Collect clickstream data"
      trigger:
        "Process is initiated when user browses the web"
      data:
        "Clickstream data"
      destination: WEBSITE
    }
    policy {
      cookies_allowed: NO
      policy_exception_justification:
        "Not implemented"
    }
  )");
}
}  // namespace

CSApiClientService::CSApiClientService(
    Profile* profile,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : profile_(profile),
      web_request_helper_(GetNetworkTrafficAnnotationTag(),
                          std::move(url_loader_factory)) {}

CSApiClientService::~CSApiClientService() = default;

void CSApiClientService::Post(std::string data, ResultCallback callback) {
  GURL api_url{base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                             "analytics.ahrefs.com", "/", "api/event"})};
  DCHECK(api_url.is_valid()) << "Invalid API Url: " << api_url.spec();

  DVLOG(0) << "|>> Sending url : " << data;

  base::Value::Dict dict;
  dict.Set("n", "pageview");
  dict.Set("u", data);
  dict.Set("k", kStagingApiKey);

  std::string profile_id = profile_->UniqueId();
  dict.Set("v", profile_id);

  std::string json_payload;
  base::JSONWriter::Write(dict, &json_payload);

  DVLOG(0) << "|>> Sending payload : " << json_payload;

  web_request_helper_.Request(kHttpMethod, api_url, json_payload, kContentType,
                              std::move(callback), {}, {});
}
