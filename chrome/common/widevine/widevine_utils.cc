// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "widevine_utils.h"

#include <string>

#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/component_updater/widevine_cdm_component_installer.h"
#include "chrome/grit/generated_resources.h"
#include "components/component_updater/component_updater_service.h"
#include "components/permissions/permission_request_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "constants.h"
#include "third_party/widevine/cdm/buildflags.h"
#include "widevine_permission_request.h"

namespace {

#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT)
void InstallWidevineOnceRegistered() {
  component_updater::ComponentUpdateService *component_update_service =  g_browser_process->component_updater();
  if (component_update_service) {
    component_update_service->MaybeThrottle(kWidevineComponentId, base::DoNothing());
  }
}
#endif

}  // namespace

void EnableWidevineCdm() {
  if (IsWidevineEnabled()) {
    return;
  }

  SetWidevineEnabled(true);
#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT)
  RegisterWidevineCdmComponentWithCallback(g_browser_process->component_updater(),
                               base::BindOnce(&InstallWidevineOnceRegistered));
#endif
}

void DisableWidevineCdm() {
  if (!IsWidevineEnabled()) {
    return;
  }

  SetWidevineEnabled(false);
}

int GetWidevinePermissionRequestTextFrangmentResourceId(bool for_restart) {
#if BUILDFLAG(IS_LINUX)
  return for_restart
             ? IDS_WIDEVINE_PERMISSION_REQUEST_TEXT_FRAGMENT_RESTART_BROWSER
             : IDS_WIDEVINE_PERMISSION_REQUEST_TEXT_FRAGMENT_INSTALL;
#elif BUILDFLAG(IS_ANDROID)
  return IDS_WIDEVINE_PERMISSION_REQUEST_TEXT_FRAGMENT_ANDROID;
#else
  return IDS_WIDEVINE_PERMISSION_REQUEST_TEXT_FRAGMENT;
#endif
}

//void RequestWidevinePermission(content::WebContents* web_contents,
//                               bool for_restart) {
//  permissions::PermissionRequestManager::FromWebContents(web_contents)
//      ->AddRequest(web_contents->GetPrimaryMainFrame(),
//                   new WidevinePermissionRequest(web_contents, for_restart));
//}

void RegisterWidevineLocalstatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kWidevineEnabled, false);
}

void RegisterWidevineProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kAskWidvineInstall, true);
}

bool IsWidevineEnabled() {
  return g_browser_process->local_state()->GetBoolean(kWidevineEnabled);
}

void SetWidevineEnabled(bool opted_in) {
  g_browser_process->local_state()->SetBoolean(kWidevineEnabled, opted_in);
}
