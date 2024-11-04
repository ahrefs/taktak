#ifndef CHROMIUM_CHAT_PAGE_HANDLER_H
#define CHROMIUM_CHAT_PAGE_HANDLER_H

#include <memory>
#include <optional>
#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/types/expected.h"
#include "chat_ui.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/side_panel/chat/api/completion_api_client.h"
#include "chrome/browser/ui/webui/side_panel/chat/chat.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace content {
    class WebContents;
    class WebUI;
}  // namespace content

class ChatPageHandler : public chat::mojom::PageHandler {
public:
    ChatPageHandler(mojo::PendingReceiver<chat::mojom::PageHandler> receiver,
                    mojo::PendingRemote<chat::mojom::Page> page,
                    ChatUI* chat_ui,
                    content::WebUI* web_ui);

    ChatPageHandler(const ChatPageHandler&) = delete;
    ChatPageHandler& operator=(const ChatPageHandler&) = delete;

    ~ChatPageHandler() override;

    void GetSiteInfo(GetSiteInfoCallback callback) override;
    void GetActionList(GetActionListCallback callback) override;
    void SubmitAction(chat::mojom::ActionType action_type) override;
    void SubmitQuery(chat::mojom::ActionType action_type, const std::string& query) override;
    void ShowUI() override;
    void CloseUI() override;

    void SetSiteInfo(chat::mojom::SiteInfoPtr site_info);

    void SubmitQueryCallback(std::string completion);
    void SubmitQueryCompletedCallback(
            base::expected<std::string, chat::mojom::APIError> result);

private:
    mojo::Receiver<chat::mojom::PageHandler> receiver_;
    mojo::Remote<chat::mojom::Page> page_;
    const raw_ptr<ChatUI> chat_ui_;
    const raw_ptr<content::WebUI> web_ui_;
    raw_ptr<content::WebContents, DanglingUntriaged> web_contents_;
    const raw_ptr<Profile> profile_;
    std::unique_ptr<CompletionApiClient> api_client_ = nullptr;
};
#endif //CHROMIUM_CHAT_PAGE_HANDLER_H
