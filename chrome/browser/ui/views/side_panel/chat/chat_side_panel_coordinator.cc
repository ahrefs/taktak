// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chat_side_panel_coordinator.h"

#include <memory>

#include "base/functional/callback.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/side_panel/chat/chat_side_panel_web_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_registry.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/base/ui_base_features.h"
#include "ui/views/vector_icons.h"

ChatSidePanelCoordinator::ChatSidePanelCoordinator(Browser* browser)
    : BrowserUserData<ChatSidePanelCoordinator>(*browser) {}

ChatSidePanelCoordinator::~ChatSidePanelCoordinator() = default;

void ChatSidePanelCoordinator::CreateAndRegisterEntry(
    SidePanelRegistry* global_registry) {
  global_registry->Register(std::make_unique<SidePanelEntry>(
      SidePanelEntry::Id::kAIChat,
      base::BindRepeating(&ChatSidePanelCoordinator::CreateChatWebView,
                          base::Unretained(this))));
}

std::unique_ptr<views::View> ChatSidePanelCoordinator::CreateChatWebView(
        SidePanelEntryScope& scope) {
  return std::make_unique<ChatSidePanelWebView>(&GetBrowser(), scope,
                                                base::RepeatingClosure());
}

void ChatSidePanelCoordinator::UpdateOpeningPanelId(SidePanelEntryId panel_id) {
  auto* browser = &GetBrowser();
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  if (browser_view) {
    auto* toolbar = browser_view->toolbar();
    if (panel_id != SidePanelEntry::Id::kAIChat) {
      toolbar->ResetHighlightForAIChatButton();
    } else {
      toolbar->AddHighlightForAIChatButton();
    }
  }
}

void ChatSidePanelCoordinator::UpdateClosingPanelId(SidePanelEntryId panel_id) {
  auto* browser = &GetBrowser();
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  if (browser_view && panel_id == SidePanelEntry::Id::kAIChat) {
    auto* toolbar = browser_view->toolbar();
    toolbar->ResetHighlightForAIChatButton();
  }
}

BROWSER_USER_DATA_KEY_IMPL(ChatSidePanelCoordinator);
