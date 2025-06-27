// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chat_side_panel_web_view.h"

#include <memory>
#include <string>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/bookmarks/bookmark_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_element_identifiers.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/view.h"
#include "ui/views/view_class_properties.h"
#include "url/gurl.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"

using SidePanelWebUIViewT_ChatUI = SidePanelWebUIViewT<ChatUI>;
BEGIN_TEMPLATE_METADATA(SidePanelWebUIViewT_ChatUI, SidePanelWebUIViewT)
END_METADATA

ChatSidePanelWebView::ChatSidePanelWebView(Browser* browser,
                                           SidePanelEntryScope& scope,
                                           base::RepeatingClosure close_cb)
    : SidePanelWebUIViewT(
          scope,
          base::BindRepeating(&ChatSidePanelWebView::UpdateActiveWebContents,
                              base::Unretained(this)),
          close_cb,
          std::make_unique<WebUIContentsWrapperT<ChatUI>>(
              GURL(chrome::kChromeUIChatURL),
              browser->profile(),
              IDS_AI_CHAT_TITLE,
              /*esc_closes_ui=*/false)),
      browser_(browser),
      weak_ptr_factory_(this) {
  SetProperty(views::kElementIdentifierKey, kChatSidePanelWebViewElementId);
  browser_->tab_strip_model()->AddObserver(this);
}

void ChatSidePanelWebView::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (GetVisible() && selection.active_tab_changed()) {
    UpdateActiveSiteInfo(tab_strip_model->GetActiveWebContents());
  }
}

void ChatSidePanelWebView::TabChangedAt(content::WebContents* contents,
                                        int index,
                                        TabChangeType change_type) {
  if (GetVisible() && index == browser_->tab_strip_model()->active_index() &&
      change_type == TabChangeType::kAll) {
    GURL url = contents->GetLastCommittedURL();
    if (last_visited_url_ != url) {
      last_visited_url_ = url;
      DVLOG(0) << " |>> ChatSidePanelWebView::TabChangedAt: " << url.spec();
      UpdateActiveSiteInfo(
          browser_->tab_strip_model()->GetWebContentsAt(index));
    }
  }
}

void ChatSidePanelWebView::UpdateActiveSiteInfo(
    content::WebContents* contents) {
  auto* controller = contents_wrapper()->GetWebUIController();
  if (!controller || !contents) {
    return;
  }

  chat::mojom::SiteInfoPtr site_info = chat::mojom::SiteInfo::New();
  site_info->title = base::UTF16ToUTF8(contents->GetTitle());

  const GURL gurl = contents->GetLastCommittedURL();
  if (gurl.SchemeIsHTTPOrHTTPS()) {
    site_info->url = gurl.spec();
    site_info->is_content_usable_in_conversations = true;
  } else {
    site_info->url = "";
    site_info->is_content_usable_in_conversations = false;
  }

  controller->GetAs<ChatUI>()->SetSiteInfo(site_info.Clone(), contents);
}

base::WeakPtr<ChatSidePanelWebView> ChatSidePanelWebView::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void ChatSidePanelWebView::UpdateActiveWebContents() {
  UpdateActiveSiteInfo(browser_->tab_strip_model()->GetActiveWebContents());
}

ChatSidePanelWebView::~ChatSidePanelWebView() = default;

BEGIN_METADATA(ChatSidePanelWebView)
END_METADATA
