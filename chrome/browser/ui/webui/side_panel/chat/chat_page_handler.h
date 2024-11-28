#ifndef CHROMIUM_CHAT_PAGE_HANDLER_H
#define CHROMIUM_CHAT_PAGE_HANDLER_H

#include <memory>
#include <optional>
#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "base/types/expected.h"
#include "chat_ui.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/side_panel/chat/api/completion_api_client.h"
#include "chrome/browser/ui/webui/side_panel/chat/chat.mojom.h"
#include "chrome/common/chat/page_content_extractor.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "page_content_extractor_helper.h"

namespace content {
    class WebContents;

    class WebUI;
}  // namespace content

class ChatPageHandler : public chat::mojom::PageHandler {
 public:
  ChatPageHandler(mojo::PendingReceiver<chat::mojom::PageHandler> receiver,
                  mojo::PendingRemote<chat::mojom::Page> page,
                  ChatUI* chat_ui,
                  content::WebUI* web_ui,
                  content::WebContents* owner_web_contents,
                  content::WebContents* chat_context_web_contents);

  ChatPageHandler(const ChatPageHandler&) = delete;

  ChatPageHandler& operator=(const ChatPageHandler&) = delete;

  ~ChatPageHandler() override;

  void GetSiteInfo(GetSiteInfoCallback callback) override;

  void GetActionList(GetActionListCallback callback) override;

  void SubmitAction(chat::mojom::ActionType action_type) override;

  void SubmitQuery(chat::mojom::ActionType action_type,
                   const std::string& query) override;

  void ShowUI() override;

  void CloseUI() override;

  void SetSiteInfo(chat::mojom::SiteInfoPtr site_info,
                   content::WebContents* contents);

  void SubmitQueryCallback(chat::mojom::ActionType action_type,
                           std::string completion);

  void SubmitQueryCompletedCallback(
      chat::mojom::ActionType action_type,
      base::expected<std::string, chat::mojom::APIErrorType> result);

  base::WeakPtr<ChatPageHandler> GetWeakPtr();

 private:
  void OnPageContentExtracted(chat::mojom::ActionType action_type,
                              const std::string& prompt,
                              std::string content);

  mojo::Receiver<chat::mojom::PageHandler> receiver_;
  mojo::Remote<chat::mojom::Page> page_;
  const raw_ptr<ChatUI> chat_ui_ = nullptr;
  raw_ptr<content::WebContents> owner_web_contents_ = nullptr;
  raw_ptr<content::WebContents> chat_context_web_contents_ = nullptr;
  const raw_ptr<Profile> profile_ = nullptr;
  std::unique_ptr<CompletionApiClient> api_client_ = nullptr;
  std::unique_ptr<PageContentExtractorHelper> page_content_extractor_helper_ =
      nullptr;
  base::WeakPtrFactory<ChatPageHandler> weak_ptr_factory_{this};
};

#endif //CHROMIUM_CHAT_PAGE_HANDLER_H
