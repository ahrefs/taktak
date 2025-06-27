// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "generated_tel_toggle_pref.h"

#include "base/feature_list.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/extensions/api/settings_private.h"
#include "components/prefs/pref_service.h"
#include "components/browsing_data/core/pref_names.h"

namespace settings_api = extensions::api::settings_private;

GeneratedTelTogglePref::GeneratedTelTogglePref(
    Profile* profile)
    : profile_(profile) {
  user_prefs_registrar_.Init(profile->GetPrefs());
  user_prefs_registrar_.Add(
      browsing_data::prefs::kTaktakTelEnabled,
      base::BindRepeating(
          &GeneratedTelTogglePref::OnSourcePreferencesChanged,
          base::Unretained(this)));
}

GeneratedTelTogglePref::~GeneratedTelTogglePref() = default;

extensions::settings_private::SetPrefResult
GeneratedTelTogglePref::SetPref(const base::Value* value) {
  if (!value->is_bool()) {
    return extensions::settings_private::SetPrefResult::PREF_TYPE_MISMATCH;
  }
  profile_->GetPrefs()->SetBoolean(
      browsing_data::prefs::kTaktakTelEnabled,
      value->GetBool());
  return extensions::settings_private::SetPrefResult::SUCCESS;
}

settings_api::PrefObject GeneratedTelTogglePref::GetPrefObject() const {
  auto* backing_preference = profile_->GetPrefs()->FindPreference(
      browsing_data::prefs::kTaktakTelEnabled);
  settings_api::PrefObject pref_object;
  pref_object.key = browsing_data::prefs::kTaktakTelEnabled;
  pref_object.type = settings_api::PrefType::kBoolean;
  pref_object.value = base::Value(backing_preference->GetValue()->GetBool());
  return pref_object;
}

void GeneratedTelTogglePref::OnSourcePreferencesChanged() {
  NotifyObservers(browsing_data::prefs::kTaktakTelEnabled);
}
