#include "cs_handler.h"

namespace cs_handler {

CSHandler::CSHandler(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  api_client_ = std::make_unique<CSApiClient>(std::move(url_loader_factory));
}

CSHandler::~CSHandler() = default;

void CSHandler::Handle(const GURL& url) {
  if (url != last_committed_url_) {
    last_committed_url_ = url;
    if (url.SchemeIsHTTPOrHTTPS()) {
      std::string url_to_submit = url.spec();
      const std::string query = url.query();
      const bool has_q_param = query.find("q=");
      const bool has_p_param = query.find("p=");

      if (!(has_q_param || has_p_param)) {
        GURL::Replacements remove_query;
        remove_query.ClearQuery();
        remove_query.ClearRef();
        url_to_submit = url.ReplaceComponents(remove_query).spec();
      }

      api_client_->Post(
          url_to_submit, base::BindOnce([](WebRequestResult result) {
            DVLOG(0) << " |>> CS post response code : "
                     << result.response_code();
            DVLOG(0) << " |>> CS post error code : " << result.error_code();
          }));
    }
  }
}
}  // namespace cs_handler
