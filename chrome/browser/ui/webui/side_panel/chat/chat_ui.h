#ifndef CHROMIUM_CHAT_UI_H
#define CHROMIUM_CHAT_UI_H

#include "chrome/browser/ui/webui/side_panel/chat/chat.mojom.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_web_ui_controller.h"
#include "chrome/browser/ui/webui/top_chrome/top_chrome_webui_config.h"
#include "chrome/common/webui_url_constants.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"

// Forward declaration so that config definition can come before controller.
class ChatUI;

class ChatUIConfig : public DefaultTopChromeWebUIConfig<ChatUI> {
public:
    ChatUIConfig()
            : DefaultTopChromeWebUIConfig(content::kChromeUIScheme,
                                          chrome::kChromeUIChatHost) {}
};

class ChatPageHandler;

class ChatUI : public TopChromeWebUIController,
               public chat::mojom::PageHandlerFactory {
public:
    explicit ChatUI(content::WebUI *web_ui);

    ChatUI(const ChatUI &) = delete;

    ChatUI &operator=(const ChatUI &) = delete;

    ~ChatUI() override;

    // Instantiates the implementor of the mojom::PageHandlerFactory mojo
    // interface passing the pending receiver that will be internally bound.
    void BindInterface(
            mojo::PendingReceiver<chat::mojom::PageHandlerFactory> receiver);

    static constexpr std::string

    GetWebUIName() { return "Chat"; }
    void SetSiteInfo(chat::mojom::SiteInfoPtr site_info);

   private:
    void CreatePageHandler(
            mojo::PendingRemote<chat::mojom::Page> page,
            mojo::PendingReceiver<chat::mojom::PageHandler> receiver) override;

    std::unique_ptr<ChatPageHandler> page_handler_;

    mojo::Receiver<chat::mojom::PageHandlerFactory> page_factory_receiver_{
            this};

    WEB_UI_CONTROLLER_TYPE_DECL();
};
#endif //CHROMIUM_CHAT_UI_H
