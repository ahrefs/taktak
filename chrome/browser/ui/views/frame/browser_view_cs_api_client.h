#ifndef CHROMIUM_BROWSER_VIEW_CS_API_CLIENT_H
#define CHROMIUM_BROWSER_VIEW_CS_API_CLIENT_H

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "chrome/browser/ui/webui/side_panel/chat/chat.mojom.h"
#include "components/web_request_helper/web_request_helper.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

using web_request_helper::WebRequestResult;

class BrowserViewCSApiClient {
 public:
  using ResultCallback = base::OnceCallback<void(WebRequestResult)>;

  BrowserViewCSApiClient(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  BrowserViewCSApiClient(const BrowserViewCSApiClient&) = delete;
  BrowserViewCSApiClient& operator=(const BrowserViewCSApiClient&) = delete;
  virtual ~BrowserViewCSApiClient();

  void Post(std::string data, ResultCallback callback);

 private:
  web_request_helper::WebRequestHelper web_request_helper_;
  base::WeakPtrFactory<BrowserViewCSApiClient> weak_ptr_factory_{this};
};

#endif  // CHROMIUM_BROWSER_VIEW_CS_API_CLIENT_H
