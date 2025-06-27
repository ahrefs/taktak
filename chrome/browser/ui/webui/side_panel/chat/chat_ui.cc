// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifdef UNSAFE_BUFFERS_BUILD
#pragma allow_unsafe_buffers
#endif

#include "chat_ui.h"

#include "chat_page_handler.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"
#include "ui/webui/webui_util.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/side_panel_chat_resources.h"
#include "chrome/grit/side_panel_chat_resources_map.h"
#include "chrome/grit/side_panel_shared_resources.h"
#include "chrome/grit/side_panel_shared_resources_map.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"
#include "ui/base/ui_base_features.h"
#include "ui/base/webui/web_ui_util.h"
#include "ui/views/style/platform_style.h"
#include "ui/webui/color_change_listener/color_change_handler.h"
#include "ui/webui/mojo_web_ui_controller.h"

namespace {

#if BUILDFLAG(IS_ANDROID)
    content::WebContents* GetActiveWebContents(content::BrowserContext* context) {
      auto tab_models = TabModelList::models();
      auto iter = base::ranges::find_if(
          tab_models, [](const auto& model) { return model->IsActiveModel(); });
      if (iter == tab_models.end()) {
        return nullptr;
      }

      auto* active_contents = (*iter)->GetActiveWebContents();
      if (!active_contents) {
        return nullptr;
      }
      DCHECK_EQ(active_contents->GetBrowserContext(), context);
      return active_contents;
    }
#endif

    Browser *GetBrowserForWebContents(content::WebContents *web_contents) {
        if (!web_contents) {
            return nullptr;
        }

        auto *browser_window =
                BrowserWindow::FindBrowserWindowWithWebContents(web_contents);
        auto *browser_view = static_cast<BrowserView *>(browser_window);
        if (!browser_view) {
            return nullptr;
        }

        return browser_view->browser();
    }

}  // namespace

ChatUI::ChatUI(content::WebUI *web_ui)
        : TopChromeWebUIController(web_ui) {
    Profile *const profile = Profile::FromWebUI(web_ui);
    content::WebUIDataSource *source = content::WebUIDataSource::CreateAndAdd(
            profile, chrome::kChromeUIChatHost);

    static constexpr webui::LocalizedString kLocalizedStrings[] = {
        {"title", IDS_AI_CHAT_TITLE},
        {"askAnything", IDS_CHAT_ASK_ANYTHING},
        {"chatAboutThisPage", IDS_CHAT_CHAT_ABOUT_THIS_PAGE},
        {"promptSummarizeThisPage", IDS_CHAT_PROMPT_SUMMARIZE_THIS_PAGE},
        {"promptExplainInSimpleLanguage",
         IDS_CHAT_PROMPT_EXPLAIN_IT_IN_SIMPLE_LANGUAGE},
        {"promptFactCheck", IDS_CHAT_PROMPT_DRAFT_FACT_CHECK},
        {"promptTranslate", IDS_CHAT_DISPLAY_PROMPT_TRANSLATE},
        {"promptSocialMediaPost", IDS_CHAT_PROMPT_DRAFT_A_SOCIAL_MEDIA_POST},
        {"translateLanguages", IDS_CHAT_TRANSLATE_LANGS},
        {"socialMedias", IDS_CHAT_SOCIAL_MEDIAS},
        {"promptExceedMaxTokenCount", IDS_CHAT_PROMPT_INPUT_EXCEED},
        {"genericError", IDS_CHAT_GENERIC_ERROR},
        {"thinking", IDS_CHAT_THINKING},
        {"doneThinking", IDS_CHAT_DONE_THINKING},
        {"enableThinking", IDS_CHAT_ENABLE_THINKING},
        {"thinkingEnabled", IDS_CHAT_THINKING_ENABLED},
    };

    for (const auto &str: kLocalizedStrings)
        webui::AddLocalizedString(source, str.name, str.id);

    webui::SetupWebUIDataSource(
            source,
            base::span(kSidePanelChatResources, kSidePanelChatResourcesSize),
            IDR_SIDE_PANEL_CHAT_CHAT_HTML);
}

ChatUI::~ChatUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(ChatUI)

void ChatUI::BindInterface(
        mojo::PendingReceiver<color_change_listener::mojom::PageHandler>
        pending_receiver) {
    color_provider_handler_ = std::make_unique<ui::ColorChangeHandler>(
            web_ui()->GetWebContents(), std::move(pending_receiver));
}

void ChatUI::BindInterface(
        mojo::PendingReceiver<chat::mojom::PageHandlerFactory> receiver) {
    page_factory_receiver_.reset();
    page_factory_receiver_.Bind(std::move(receiver));
}

void ChatUI::CreatePageHandler(
        mojo::PendingRemote<chat::mojom::Page> page,
        mojo::PendingReceiver<chat::mojom::PageHandler> receiver) {
    DCHECK(page);
    // ShowUI() is called before creating the PageHandler.
    // This ensures the WebContents is added to a Browser,
    // allowing us to provide the Browser reference to the PageHandler.
    if (embedder_) {
        embedder_->ShowUI();
    }

    content::WebContents *web_contents = nullptr;
#if !BUILDFLAG(IS_ANDROID)
    Browser* browser = GetBrowserForWebContents(web_ui()->GetWebContents());
    if (!browser) {
      return;
    }

    TabStripModel* tab_strip_model = browser->tab_strip_model();
    DCHECK(tab_strip_model);
    web_contents = tab_strip_model->GetActiveWebContents();
#else
    web_contents = GetActiveWebContents(profile_);
#endif
    if (web_contents == web_ui()->GetWebContents()) {
        web_contents = nullptr;
    }
    page_handler_ = std::make_unique<ChatPageHandler>(
            std::move(receiver), std::move(page), this, web_ui(),
            web_ui()->GetWebContents(), web_contents);
}

void ChatUI::SetSiteInfo(chat::mojom::SiteInfoPtr site_info, content::WebContents *contents) {
    if (page_handler_) {
        page_handler_->SetSiteInfo(std::move(site_info), contents);
    }
}
