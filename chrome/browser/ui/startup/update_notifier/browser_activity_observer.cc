// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "browser_activity_observer.h"
#include "chrome/browser/ui/browser_list.h"
#include "update_notifier_prompt_manager.h"

BrowserActivityObserver::BrowserActivityObserver() {
  BrowserList::AddObserver(this);
  last_inactive_time_ = base::Time::Now();
}

BrowserActivityObserver::~BrowserActivityObserver() {
  BrowserList::RemoveObserver(this);
}

void BrowserActivityObserver::OnBrowserNoLongerActive(Browser *browser) {
  last_inactive_time_ = base::Time::Now();
}

void BrowserActivityObserver::OnBrowserSetLastActive(Browser* browser) {
  const int kInactiveDurationThresholdMinutes = 90;
  base::Time now = base::Time::Now();
  base::TimeDelta inactive_duration = now - last_inactive_time_;

  if (inactive_duration >= base::Minutes(kInactiveDurationThresholdMinutes)) {
    UpdateNotifierPromptManager::GetInstance()->MaybeShowPrompt();
  }

  last_inactive_time_ = now;
}
