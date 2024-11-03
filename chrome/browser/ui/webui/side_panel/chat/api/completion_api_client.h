#ifndef CHROMIUM_COMPLETION_API_CLIENT_H
#define CHROMIUM_COMPLETION_API_CLIENT_H

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {

using api_request_helper::APIRequestResult;

class CompletionApiClient {
 public:
  using GenerationResult = base::expected<std::string, mojom::APIError>;
  using GenerationDataCallback = base::RepeatingCallback<void(std::string)>;
  using GenerationCompletedCallback =
      base::OnceCallback<void(GenerationResult)>;

  CompletionApiClient(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  CompletionApiClient(const CompletionApiClient&) = delete;
  CompletionApiClient& operator=(const CompletionApiClient&) = delete;
  virtual ~CompletionApiClient();

  // This function queries both types of APIs: SSE and non-SSE.
  // In non-SSE cases, only the data_completed_callback will be triggered.
  virtual void QueryPrompt(
      const std::string& prompt,
      GenerationCompletedCallback data_completed_callback,
      GenerationDataCallback data_received_callback = base::NullCallback());
  // Clears all in-progress requests
  void ClearAllQueries();

 private:
  void OnQueryDataReceived(GenerationDataCallback callback,
                           base::expected<base::Value, std::string> result);
  void OnQueryCompleted(std::optional<CredentialCacheEntry> credential,
                        GenerationCompletedCallback callback,
                        APIRequestResult result);

  const base::flat_set<std::string_view> stop_sequences_;
  api_request_helper::APIRequestHelper api_request_helper_;

  base::WeakPtrFactory<CompletionApiClient> weak_ptr_factory_{this};
};

}  // namespace ai_chat
#endif  // CHROMIUM_COMPLETION_API_CLIENT_H
