// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "update_notifier_prompt_manager.h"

#include <memory>

#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/metrics/histogram_functions.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/startup/default_browser_prompt/default_browser_infobar_delegate.h"
#include "chrome/browser/ui/startup/default_browser_prompt/default_browser_prompt_prefs.h"
#include "chrome/browser/ui/startup/update_notifier/update_notifier_api_client.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/common/pref_names.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "components/infobars/core/infobar.h"
#include "components/prefs/pref_service.h"
#include "components/web_request_helper/web_request_helper.h"
#include "content/public/browser/web_contents.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "update_notifier_infobar_delegate.h"
#include "url/gurl.h"
#include "base/values.h"


namespace {
class SharedURLLoaderFactory;
}

// static
UpdateNotifierPromptManager* UpdateNotifierPromptManager::GetInstance() {
  return base::Singleton<UpdateNotifierPromptManager>::get();
}

void UpdateNotifierPromptManager::InitTabStripTracker() {
  browser_tab_strip_tracker_ =
      std::make_unique<BrowserTabStripTracker>(this, this);
  // This will trigger a call to `OnTabStripModelChanged`, which will create
  // the info bar.
  browser_tab_strip_tracker_->Init();
}

void UpdateNotifierPromptManager::MaybeShowPrompt() {
#if BUILDFLAG(IS_ANDROID)
  NOTREACHED() << "Unsupported platforms for showing updater prompts.";
#else

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory =
      g_browser_process->system_network_context_manager()
          ->GetSharedURLLoaderFactory();
  if (!api_client_) {
    api_client_ = std::make_unique<UpdateNotifierApiClient>(
        std::move(url_loader_factory));
  }
  api_client_->Post(
      "", base::BindOnce(&UpdateNotifierPromptManager::OnCheckNewerVersion,
                         base::Unretained(this)));

#endif  // BUILDFLAG(IS_ANDROID)
}

void UpdateNotifierPromptManager::CloseAllPrompts(CloseReason close_reason) {
#if BUILDFLAG(IS_ANDROID)
  NOTREACHED() << "Unsupported platforms for showing updater prompts.";
#else
  CloseAllInfoBars();
#endif
}

UpdateNotifierPromptManager::UpdateNotifierPromptManager() = default;

UpdateNotifierPromptManager::~UpdateNotifierPromptManager() = default;

void UpdateNotifierPromptManager::OnCheckNewerVersion(WebRequestResult result) {
  if (result.response_code() != 200 ) {
    DVLOG(0) << __func__ << " |>> Failed checking new version with error code: "
             << result.response_code();
    return;
  }
  DVLOG(0) << __func__
           << " |>> Success checking new version with response code: "
           << result.response_code();
  if (result.value_body().is_dict() ) {
    const base::Value::Dict &dict = result.value_body().GetDict();
    const std::optional<bool> success = dict.FindBool("success");
    DVLOG(0) << __func__ << " |>> success: " << success.value();
    if (success.has_value() && success.value()) {
      const std::optional<bool> data = dict.FindBool("data");
      if (data.has_value() && data.value()) {
        DVLOG(0) << __func__ << " |>> data: " << data.value();
        InitTabStripTracker();
      }
    }
  }
}

void UpdateNotifierPromptManager::CreateInfoBarForWebContents(
    content::WebContents* web_contents,
    Profile* profile) {
  // Ensure that an infobar hasn't already been created.
  CHECK(!infobars_.contains(web_contents));

  infobars::InfoBar* infobar = UpdateNotifierInfoBarDelegate::Create(
      infobars::ContentInfoBarManager::FromWebContents(web_contents), profile);

  if (infobar == nullptr) {
    // Infobar may be null if `InfoBarManager::ShouldShowInfoBar` returns false,
    // in which case this function should do nothing. One case where this can
    // happen is if the --headless command  line switch is present.
    return;
  }

  infobars_[web_contents] = infobar;

  static_cast<ConfirmInfoBarDelegate*>(infobar->delegate())->AddObserver(this);

  auto* infobar_manager =
      infobars::ContentInfoBarManager::FromWebContents(web_contents);
  infobar_manager->AddObserver(this);
}

void UpdateNotifierPromptManager::CloseAllInfoBars() {
  browser_tab_strip_tracker_.reset();

  for (const auto& infobars_entry : infobars_) {
    infobars_entry.second->owner()->RemoveObserver(this);
    infobars_entry.second->RemoveSelf();
  }

  infobars_.clear();
}

bool UpdateNotifierPromptManager::ShouldTrackBrowser(Browser* browser) {
  return browser->is_type_normal() &&
         !browser->profile()->IsIncognitoProfile() &&
         !browser->profile()->IsGuestSession();
}

void UpdateNotifierPromptManager::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (change.type() == TabStripModelChange::kInserted) {
    for (const auto& contents : change.GetInsert()->contents) {
      if (!base::Contains(infobars_, contents.contents)) {
        CreateInfoBarForWebContents(contents.contents,
                                    tab_strip_model->profile());
      }
    }
  }
}

void UpdateNotifierPromptManager::OnInfoBarRemoved(infobars::InfoBar* infobar,
                                                   bool animate) {
  auto infobars_entry = std::ranges::find(
      infobars_, infobar, &decltype(infobars_)::value_type::second);
  if (infobars_entry == infobars_.end()) {
    return;
  }

  infobar->owner()->RemoveObserver(this);
  infobars_.erase(infobars_entry);
  static_cast<ConfirmInfoBarDelegate*>(infobar->delegate())
      ->RemoveObserver(this);

  if (user_initiated_info_bar_close_pending_.has_value()) {
    CloseAllPrompts(user_initiated_info_bar_close_pending_.value());
    user_initiated_info_bar_close_pending_.reset();
  }
}

void UpdateNotifierPromptManager::OnAccept() {
  user_initiated_info_bar_close_pending_ = CloseReason::kAccept;
}

void UpdateNotifierPromptManager::OnDismiss() {
  user_initiated_info_bar_close_pending_ = CloseReason::kDismiss;
}
