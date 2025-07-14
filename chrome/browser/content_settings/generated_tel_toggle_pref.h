// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef CHROMIUM_SRC_CHROME_BROWSER_CONTENT_SETTINGS_GENERATED_TEL_TOGGLE_PREF_H_
#define CHROMIUM_SRC_CHROME_BROWSER_CONTENT_SETTINGS_GENERATED_TEL_TOGGLE_PREF_H_

#include "base/memory/raw_ptr.h"
#include "chrome/browser/extensions/api/settings_private/generated_pref.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_change_registrar.h"

class GeneratedTelTogglePref :  public extensions::settings_private::GeneratedPref {
 public:
  explicit GeneratedTelTogglePref(Profile* profile);
  ~GeneratedTelTogglePref() override;

  // Generated Preference Interface.
  extensions::settings_private::SetPrefResult SetPref(
      const base::Value* value) override;
  extensions::api::settings_private::PrefObject GetPrefObject() const override;

  // Fired when preferences used to generate this preference are changed.
  void OnSourcePreferencesChanged();

 private:
  // Non-owning pointer to the profile this preference is generated for.
  const raw_ptr<Profile> profile_;
  PrefChangeRegistrar user_prefs_registrar_;
};

#endif //CHROMIUM_SRC_CHROME_BROWSER_CONTENT_SETTINGS_GENERATED_TEL_TOGGLE_PREF_H_
