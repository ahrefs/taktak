// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chat_side_panel_web_view.h"

#include <memory>
#include <string>

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
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
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_types.h"
#include "ui/views/view.h"
#include "ui/views/view_class_properties.h"
#include "url/gurl.h"

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

ChatSidePanelWebView::~ChatSidePanelWebView() {
  if (browser_) {
    browser_->tab_strip_model()->RemoveObserver(this);
  }
  Observe(nullptr);
}

void ChatSidePanelWebView::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
/*
  if (GetVisible() && selection.active_tab_changed()) {
    UpdateActiveSiteInfo(tab_strip_model->GetActiveWebContents());
    TryRunDescriptionScript();
  }
*/
}

void ChatSidePanelWebView::TabChangedAt(content::WebContents* contents,
                                        int index,
                                        TabChangeType change_type) {
  if (GetVisible() && index == browser_->tab_strip_model()->active_index() &&
      change_type == TabChangeType::kAll) {
    GURL url = contents->GetLastCommittedURL();
    if (last_visited_url_ != url) {
      last_visited_url_ = url;
      DVLOG(0) << __func__ << " |>> Tab changed to " << url.spec();
      UpdateActiveSiteInfo(
          browser_->tab_strip_model()->GetWebContentsAt(index));
    }
    //active_tab_ = browser_->tab_strip_model()->GetWebContentsAt(index);
    //TryRunDescriptionScript();

  }
}

// ---- WebContentsObserver (for the *active tab*) ----
void ChatSidePanelWebView::DidFinishNavigation(content::NavigationHandle* nav) {
  VLOG(0) << __func__ << " |>> Navigation finished to "
           << nav->GetURL().spec();
  if (nav->IsInPrimaryMainFrame() && nav->HasCommitted()) {
      VLOG(0) << __func__ << " |>> Navigation finished to "
               << nav->GetURL().spec();
      active_tab_ = nav->GetWebContents();
      TryRunDescriptionScript();
  }
}

void ChatSidePanelWebView::DocumentOnLoadCompletedInPrimaryMainFrame() {
   TryRunDescriptionScript();
}

void ChatSidePanelWebView::PrimaryPageChanged(content::Page& page) {
   TryRunDescriptionScript();
}

void ChatSidePanelWebView::AttachToActiveTab() {
  Observe(nullptr);
  active_tab_ = browser_->tab_strip_model()->GetActiveWebContents();
  if (!active_tab_) {
    return;
  }
  Observe(active_tab_);
  TryRunDescriptionScript();
}

void ChatSidePanelWebView::TryRunDescriptionScript() {
  if (!active_tab_) {
    return;
  }
  content::RenderFrameHost* rfh = active_tab_->GetPrimaryMainFrame();
  if (!rfh) {
    return;  // Not ready yet.
  }
  if (!rfh->IsActive() || !rfh->IsRenderFrameLive()) {
    return;
  }

  static const char16_t kScript[] =
      uR"JS(
      (() => {
        const pick = (sel, attr='content') => {
          const el = document.querySelector(sel);
          return el ? (attr === 'text' ? el.textContent.trim()
                                       : (el.getAttribute(attr) || '').trim())
                    : '';
        };
        let desc = pick('meta[name="description"]')
                || pick('meta[name="Description"]')
                || pick('meta[property="og:description"]')
                || pick('meta[name="twitter:description"]');
        if (!desc) {
          const p = Array.from(document.querySelectorAll('p'))
            .map(n => (n.textContent || '').trim())
            .find(t => t && t.length > 40);
          if (p) desc = p.slice(0, 320);
        }
        if (desc.length > 500) desc = desc.slice(0, 500);
        return desc;
      })()
    )JS";

  rfh->ExecuteJavaScript(
      std::u16string(kScript),
      base::BindOnce(&ChatSidePanelWebView::OnDescriptionJSResult,
                     GetWeakPtr()));
}

void ChatSidePanelWebView::OnDescriptionJSResult(base::Value result) {
  auto* controller = contents_wrapper()->GetWebUIController();
  if (!controller) {
    return;
  }
  std::string desc;
  if (result.is_string()) {
    desc = result.GetString();
  }

  if (!site_info_) {
    site_info_ = chat::mojom::SiteInfo::New();
  }
  site_info_->description = std::move(desc);
  controller->GetAs<ChatUI>()->SetSiteInfo(std::move(site_info_), active_tab_);
}

void ChatSidePanelWebView::UpdateActiveSiteInfo(
    content::WebContents* contents) {
  auto* controller = contents_wrapper()->GetWebUIController();
  if (!controller || !contents) {
    return;
  }

  chat::mojom::SiteInfoPtr siteinfo = chat::mojom::SiteInfo::New();
  siteinfo->title = base::UTF16ToUTF8(contents->GetTitle());

  const GURL gurl = contents->GetLastCommittedURL();
  if (gurl.SchemeIsHTTPOrHTTPS()) {
    siteinfo->url = gurl.spec();
    siteinfo->is_content_usable_in_conversations = true;
  } else {
    siteinfo->url = "";
    siteinfo->is_content_usable_in_conversations = false;
  }

  site_info_ = std::move(siteinfo);
  controller->GetAs<ChatUI>()->SetSiteInfo(std::move(site_info_), active_tab_);
}

base::WeakPtr<ChatSidePanelWebView> ChatSidePanelWebView::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void ChatSidePanelWebView::UpdateActiveWebContents() {
 // UpdateActiveSiteInfo(browser_->tab_strip_model()->GetActiveWebContents());
}

BEGIN_METADATA(ChatSidePanelWebView)
END_METADATA
