// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_UPDATE_NOTIFIER_UPDATENOTIFIERPROMPTMANAGER_H_
#define CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_UPDATE_NOTIFIER_UPDATENOTIFIERPROMPTMANAGER_H_

#include <map>

#include "base/memory/raw_ptr.h"
#include "base/memory/singleton.h"
#include "base/timer/timer.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/browser_tab_strip_tracker.h"
#include "chrome/browser/ui/browser_tab_strip_tracker_delegate.h"
#include "chrome/browser/ui/tabs/tab_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "components/infobars/core/infobar.h"
#include "components/infobars/core/infobar_manager.h"
#include "content/public/browser/web_contents.h"
#include "update_notifier_api_client.h"

using web_request_helper::WebRequestResult;

class UpdateNotifierPromptManager : public BrowserTabStripTrackerDelegate,
                                    public TabStripModelObserver,
                                    public infobars::InfoBarManager::Observer,
                                    public ConfirmInfoBarDelegate::Observer {
 public:
  UpdateNotifierPromptManager(const UpdateNotifierPromptManager&) = delete;
  UpdateNotifierPromptManager& operator=(const UpdateNotifierPromptManager&) =
      delete;

  enum class CloseReason {
    kAccept,
    kDismiss,
  };

  static UpdateNotifierPromptManager* GetInstance();

  // This will trigger the showing of the info bar.
  void InitTabStripTracker();

  void MaybeShowPrompt();

  void CloseAllPrompts(CloseReason close_reason);

 private:
  friend struct base::DefaultSingletonTraits<UpdateNotifierPromptManager>;

  UpdateNotifierPromptManager();
  ~UpdateNotifierPromptManager() override;

  void CreateInfoBarForWebContents(content::WebContents* contents,
                                   Profile* profile);

  void CloseAllInfoBars();

  // BrowserTabStripTrackerDelegate
  bool ShouldTrackBrowser(Browser* browser) override;

  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  // InfoBarManager::Observer:
  void OnInfoBarRemoved(infobars::InfoBar* infobar, bool animate) override;

  // ConfirmInfoBarDelegate::Observer
  void OnAccept() override;
  void OnDismiss() override;

  void OnCheckNewerVersion(WebRequestResult result);

  std::unique_ptr<BrowserTabStripTracker> browser_tab_strip_tracker_;

  std::map<content::WebContents*, raw_ptr<infobars::InfoBar, CtnExperimental>>
      infobars_;

  std::optional<CloseReason> user_initiated_info_bar_close_pending_;

  std::unique_ptr<UpdateNotifierApiClient> api_client_;
};

#endif  // CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_UPDATE_NOTIFIER_UPDATENOTIFIERPROMPTMANAGER_H_
