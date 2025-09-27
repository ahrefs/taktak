// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef CHROMIUM_SRC_CHROME_COMMON_WIDEVINE_WIDEVINE_UTILS_H_
#define CHROMIUM_SRC_CHROME_COMMON_WIDEVINE_WIDEVINE_UTILS_H_

namespace content {
class WebContents;
}  // namespace content

class PrefRegistrySimple;

void EnableWidevineCdm();
void DisableWidevineCdm();
int GetWidevinePermissionRequestTextFrangmentResourceId(bool for_restart);
void RegisterWidevineLocalstatePrefs(PrefRegistrySimple* registry);
void RegisterWidevineProfilePrefs(PrefRegistrySimple* registry);
bool IsWidevineEnabled();
void SetWidevineEnabled(bool opted_in);

#endif  // CHROMIUM_SRC_CHROME_COMMON_WIDEVINE_WIDEVINE_UTILS_H_
