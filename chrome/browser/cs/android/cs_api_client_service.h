#ifndef CHROME_BROWSER_CS_CS_API_CLIENT_SERVICE_H_
#define CHROME_BROWSER_CS_CS_API_CLIENT_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/web_request_helper/web_request_helper.h"

namespace network {
class SharedURLLoaderFactory;
} // namespace network

using web_request_helper::WebRequestResult;

class CSApiClientService : public KeyedService {
  public:
    CSApiClientService(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
    ~CSApiClientService() override;
    CSApiClientService(const CSApiClientService&) = delete;
    CSApiClientService& operator=(const CSApiClientService&) = delete;

    using ResultCallback = base::OnceCallback<void(WebRequestResult)>;

    void Post(std::string data, ResultCallback callback);

  private:
    web_request_helper::WebRequestHelper web_request_helper_;
    base::WeakPtrFactory<CSApiClientService> weak_ptr_factory_{this};
};
#endif // CHROME_BROWSER_CS_CS_API_CLIENT_SERVICE_H_
