#include "chat_ui.h"
#include "chat_page_handler.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "chrome/common/webui_url_constants.h"
#include "content/public/common/url_constants.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/webui_config.h"
#include "chrome/grit/side_panel_chat_resources.h"
#include "chrome/grit/side_panel_chat_resources_map.h"
#include "ui/webui/mojo_web_ui_controller.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/grit/generated_resources.h"

#pragma allow_unsafe_buffers

ChatUI::ChatUI(content::WebUI* web_ui)
        : TopChromeWebUIController(web_ui) {
    Profile* const profile = Profile::FromWebUI(web_ui);
    content::WebUIDataSource* source = content::WebUIDataSource::CreateAndAdd(
            profile, chrome::kChromeUIChatHost);

    static constexpr webui::LocalizedString kLocalizedStrings[] = {
            {"title", IDS_AI_CHAT_TITLE},
            {"askAnything", IDS_CHAT_ASK_ANYTHING},
    };
    for (const auto& str : kLocalizedStrings)
        webui::AddLocalizedString(source, str.name, str.id);

    webui::SetupWebUIDataSource(
            source,
            base::make_span(kSidePanelChatResources, kSidePanelChatResourcesSize),
            IDR_SIDE_PANEL_CHAT_CHAT_HTML);
}

ChatUI::~ChatUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(ChatUI)

void ChatUI::BindInterface(
        mojo::PendingReceiver<chat::mojom::PageHandlerFactory> receiver) {
    page_factory_receiver_.reset();
    page_factory_receiver_.Bind(std::move(receiver));
}

void ChatUI::CreatePageHandler(
        mojo::PendingRemote<chat::mojom::Page> page,
        mojo::PendingReceiver<chat::mojom::PageHandler> receiver) {
    DCHECK(page);
    page_handler_ = std::make_unique<ChatPageHandler>(
        std::move(receiver), std::move(page), this, web_ui());
}

void ChatUI::SetSiteInfo(chat::mojom::SiteInfoPtr site_info) {
  if (page_handler_) {
    page_handler_->SetSiteInfo(std::move(site_info));
  }
}
