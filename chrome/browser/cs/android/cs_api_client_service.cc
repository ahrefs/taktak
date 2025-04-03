#include "cs_api_client_service.h"

#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace {

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
} // namespace

CSApiClientService::CSApiClientService(
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
  : web_request_helper_(GetNetworkTrafficAnnotationTag(),
                        std::move(url_loader_factory)) {}

CSApiClientService::~CSApiClientService() = default;

void CSApiClientService::Post(std::string data, ResultCallback callback) {
  // TODO
}
