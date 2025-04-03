#ifndef CHROME_BROWSER_CS_CS_API_CLIENT_SERVICE_FACTORY_H_
#define CHROME_BROWSER_CS_CS_API_CLIENT_SERVICE_FACTORY_H_

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

namespace content {
class BrowserContext;
} // namespace content

class CSApiClientService;

namespace cs_api_client_module {
// Factory to create CSApiClientService sper regular profile
// Null pointer will be returned for incognito profile
class CSApiClientServiceFactory : public ProfileKeyedServiceFactory {
  public:
    static CSApiClientService* GetForBrowserContext(
      content::BrowserContext* context);
    static CSApiClientServiceFactory* GetInstance();

    CSApiClientServiceFactory(const CSApiClientServiceFactory&) = delete;
    CSApiClientServiceFactory& operator=(const CSApiClientServiceFactory&) = delete;

  private:
    friend class base::NoDestructor<CSApiClientServiceFactory>;

    CSApiClientServiceFactory();
    ~CSApiClientServiceFactory() override;

    std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

} // namespace cs_api_client_module
#endif // CHROME_BROWSER_CS_CS_API_CLIENT_SERVICE_FACTORY_H_
