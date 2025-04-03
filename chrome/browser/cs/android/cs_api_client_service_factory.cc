#include "cs_api_client_service_factory.h"

#include <utility>

#include "chrome/browser/cs/android/cs_api_client_service.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"

namespace cs_api_client_module {
// static
CSApiClientService* CSApiClientServiceFactory::GetForBrowserContext(
    content::BrowserContext* browser_context) {
  return static_cast<CSApiClientService*>(
      GetInstance()->GetServiceForBrowserContext(browser_context, true));
}

// static
CSApiClientServiceFactory* CSApiClientServiceFactory::GetInstance() {
  static base::NoDestructor<CSApiClientServiceFactory> instance;
  return instance.get();
}

CSApiClientServiceFactory::CSApiClientServiceFactory()
    : ProfileKeyedServiceFactory(
          "CSApiClientServiceFactory",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOriginalOnly)
              // TODO(crbug.com/40257657): Check if this service is needed in Guest mode
              .WithGuest(ProfileSelection::kOriginalOnly)
              .Build()) {}

CSApiClientServiceFactory::~CSApiClientServiceFactory() = default;

std::unique_ptr<KeyedService>
CSApiClientServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  auto url_loader_factory = context->GetDefaultStoragePartition()
                                ->GetURLLoaderFactoryForBrowserProcess();
  return std::make_unique<CSApiClientService>(profile, url_loader_factory);
}

} // namespace cs_api_client_module
