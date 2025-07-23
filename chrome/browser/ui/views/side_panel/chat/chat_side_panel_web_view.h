// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef CHROMIUM_CHAT_SIDE_PANEL_WEB_VIEW_H
#define CHROMIUM_CHAT_SIDE_PANEL_WEB_VIEW_H

#include <memory>
#include <optional>
#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "chrome/browser/ui/webui/side_panel/chat/chat.mojom.h"
#include "chrome/browser/ui/webui/side_panel/chat/chat_ui.h"
#include "chrome/common/page_content_extractor/page_content_extractor.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/view.h"

class Browser;

class ChatSidePanelWebView : public SidePanelWebUIViewT<ChatUI>,
                             public TabStripModelObserver {
  using SidePanelWebUIViewT_ChatUI = SidePanelWebUIViewT<ChatUI>;
  METADATA_HEADER(ChatSidePanelWebView, SidePanelWebUIViewT_ChatUI)
 public:
  ChatSidePanelWebView(Browser* browser,
                       SidePanelEntryScope& scope,
                       base::RepeatingClosure close_cb);
  ChatSidePanelWebView(const ChatSidePanelWebView&) = delete;
  ChatSidePanelWebView& operator=(const ChatSidePanelWebView&) = delete;
  ~ChatSidePanelWebView() override;

  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  void TabChangedAt(content::WebContents* contents,
                    int index,
                    TabChangeType change_type) override;

  void UpdateActiveSiteInfo(content::WebContents* contents);
  void UpdateActiveWebContents();
  base::WeakPtr<ChatSidePanelWebView> GetWeakPtr();

 private:
  const raw_ptr<Browser> browser_;
  GURL last_visited_url_;
  base::WeakPtrFactory<ChatSidePanelWebView> weak_ptr_factory_{this};
};
#endif  // CHROMIUM_CHAT_SIDE_PANEL_WEB_VIEW_H
