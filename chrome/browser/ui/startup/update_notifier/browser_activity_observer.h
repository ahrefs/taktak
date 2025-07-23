// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_UPDATE_NOTIFIER_BROWSER_ACTIVITY_OBSERVER_H_
#define CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_UPDATE_NOTIFIER_BROWSER_ACTIVITY_OBSERVER_H_

#include "base/time/time.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/browser.h"

class BrowserActivityObserver : public BrowserListObserver {
 public:
  BrowserActivityObserver();
  ~BrowserActivityObserver() override;

  void OnBrowserNoLongerActive(Browser* browser) override;

  void OnBrowserSetLastActive(Browser* browser) override;

 private:

  base::Time last_inactive_time_;
  base::WeakPtrFactory<BrowserActivityObserver> weak_factory_{this};
};

#endif //CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_UPDATE_NOTIFIER_BROWSER_ACTIVITY_OBSERVER_H_
