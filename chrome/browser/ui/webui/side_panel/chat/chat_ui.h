// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef CHROMIUM_CHAT_UI_H
#define CHROMIUM_CHAT_UI_H

#include <string>
#include <unordered_map>

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
#include "ui/webui/resources/cr_components/color_change_listener/color_change_listener.mojom.h"

class ChatPageHandler;

namespace ui {
class ColorChangeHandler;
}

class ChatUI;

class ChatUIConfig : public DefaultTopChromeWebUIConfig<ChatUI> {
public:
    ChatUIConfig()
            : DefaultTopChromeWebUIConfig(content::kChromeUIScheme,
                                          chrome::kChromeUIChatHost) {}
};

class ChatUI : public TopChromeWebUIController,
               public chat::mojom::PageHandlerFactory {
public:
    explicit ChatUI(content::WebUI *web_ui);

    ChatUI(const ChatUI &) = delete;

    ChatUI &operator=(const ChatUI &) = delete;

    ~ChatUI() override;

    void BindInterface(
        mojo::PendingReceiver<color_change_listener::mojom::PageHandler>
            pending_receiver);

    void BindInterface(
            mojo::PendingReceiver<chat::mojom::PageHandlerFactory> receiver);

    // Set by WebUIContentsWrapperT.TopChromeWebUIController provides default
    // implementation for this but we don't use it.
    void set_embedder(
        base::WeakPtr<TopChromeWebUIController::Embedder> embedder) {
      embedder_ = embedder;
    }

    static constexpr std::string GetWebUIName() { return "Chat"; }

    void SetSiteInfo(chat::mojom::SiteInfoPtr site_info, content::WebContents* contents );

   private:
    void CreatePageHandler(
            mojo::PendingRemote<chat::mojom::Page> page,
            mojo::PendingReceiver<chat::mojom::PageHandler> receiver) override;

    std::unique_ptr<ui::ColorChangeHandler> color_provider_handler_;
    std::unique_ptr<ChatPageHandler> page_handler_;

    mojo::Receiver<chat::mojom::PageHandlerFactory> page_factory_receiver_{
            this};

    base::WeakPtr<TopChromeWebUIController::Embedder> embedder_;

    WEB_UI_CONTROLLER_TYPE_DECL();
};
#endif //CHROMIUM_CHAT_UI_H
