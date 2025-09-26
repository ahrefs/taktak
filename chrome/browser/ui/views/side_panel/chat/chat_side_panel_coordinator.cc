// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chat_side_panel_coordinator.h"

#include <memory>

#include "base/check_deref.h"
#include "base/functional/callback.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
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

namespace {
    std::unique_ptr<views::View> CreateChatSidePanelWebView(
            Profile* profile,
            TabStripModel* tab_strip_model,
            SidePanelEntryScope& scope) {
      return std::make_unique<ChatSidePanelWebView>(profile, tab_strip_model, scope,
                                                    base::RepeatingClosure());
    }

    Browser* GetBrowserForProfile(Profile* profile) {
      for (Browser* browser : *BrowserList::GetInstance()) {
        if (browser->profile() == profile) {
          return browser;
        }
      }
      return nullptr;
    }
}  // namespace

ChatSidePanelCoordinator::ChatSidePanelCoordinator(
        Profile* profile,
        TabStripModel* tab_strip_model)
        : profile_(CHECK_DEREF(profile)),
          tab_strip_model_(CHECK_DEREF(tab_strip_model)) {}

ChatSidePanelCoordinator::~ChatSidePanelCoordinator() = default;

void ChatSidePanelCoordinator::CreateAndRegisterEntry(
        SidePanelRegistry* global_registry) {
  global_registry->Register(std::make_unique<SidePanelEntry>(
          SidePanelEntry::Key(SidePanelEntry::Id::kAIChat),
          base::BindRepeating(&CreateChatSidePanelWebView, &profile_.get(),
                              &tab_strip_model_.get()),
          /*default_content_width_callback=*/base::NullCallback()));
}

void ChatSidePanelCoordinator::UpdateOpeningPanelId(SidePanelEntryId panel_id) {
  auto* browser = GetBrowserForProfile(&profile_.get());
  if (!browser) {
    return;
  }
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
  auto* browser = GetBrowserForProfile(&profile_.get());
  if (!browser) {
    return;
  }
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
  if (browser_view && panel_id == SidePanelEntry::Id::kAIChat) {
    auto* toolbar = browser_view->toolbar();
    toolbar->ResetHighlightForAIChatButton();
  }
}
