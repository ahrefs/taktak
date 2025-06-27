// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "drm_tab_helper.h"

#include <utility>
#include <vector>

#include "chrome/common/widevine/widevine_utils.h"
#include "chrome/common/widevine/constants.h"
#include "base/containers/contains.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "components/update_client/crx_update_item.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_handle.h"

#if !BUILDFLAG(IS_ANDROID)
#include "base/check_is_test.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#endif

using component_updater::ComponentUpdateService;

namespace {
#if !BUILDFLAG(IS_ANDROID)
bool IsAlreadyRegistered(ComponentUpdateService* cus) {
  return base::Contains(cus->GetComponentIDs(), kWidevineComponentId);
}
#if !BUILDFLAG(IS_LINUX)
content::WebContents* GetActiveWebContents() {
  if (Browser* browser = chrome::FindLastActive())
    return browser->tab_strip_model()->GetActiveWebContents();
  return nullptr;
}

void ReloadIfActive(content::WebContents* web_contents) {
  if (GetActiveWebContents() == web_contents)
    web_contents->GetController().Reload(content::ReloadType::NORMAL, false);
}
#endif  // !BUILDFLAG(IS_LINUX)
#endif  // !BUILDFLAG(IS_ANDROID)
}  // namespace

DrmTabHelper::DrmTabHelper(content::WebContents* contents)
    : WebContentsObserver(contents),
      content::WebContentsUserData<DrmTabHelper>(*contents),
      taktak_drm_receivers_(contents, this) {
#if !BUILDFLAG(IS_ANDROID)
  auto* updater = g_browser_process->component_updater();
  if (updater) {
    if (!IsAlreadyRegistered(updater)) {
      observer_.Observe(updater);
    }
  } else {
    CHECK_IS_TEST();
  }
#endif
}

DrmTabHelper::~DrmTabHelper() = default;

// static
void DrmTabHelper::BindTaktakDRM(
    mojo::PendingAssociatedReceiver<taktak_drm::mojom::TaktakDRM> receiver,
    content::RenderFrameHost* rfh) {
  auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);
  if (!web_contents) {
    return;
  }

  auto* tab_helper = DrmTabHelper::FromWebContents(web_contents);
  if (!tab_helper) {
    return;
  }
  tab_helper->taktak_drm_receivers_.Bind(rfh, std::move(receiver));
}

bool DrmTabHelper::ShouldShowWidevineOptIn() const {
#if BUILDFLAG(IS_LINUX) && !defined(ARCH_CPU_X86_64)
  // On non-x64 Linux, Widevine is not publicly available. This point is a
  // convenient single place for turning this class into a no-op:
  return false;
#else
  PrefService* prefs =
      static_cast<Profile*>(web_contents()->GetBrowserContext())->GetPrefs();
  const bool is_widevine_enabled = IsWidevineEnabled();
  const bool is_widevine_installed = prefs->GetBoolean(kAskWidvineInstall);
  if (is_widevine_enabled || !is_widevine_installed) {
    return false;
  }
  return is_widevine_requested_;
#endif  // BUILDFLAG(IS_LINUX) && !defined(ARCH_CPU_X86_64)
}

void DrmTabHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }
  is_widevine_requested_ = false;
  is_permission_requested_ = false;
}

void DrmTabHelper::HandleWidevineKeySystemRequest() {
  is_widevine_requested_ = true;
#if BUILDFLAG(IS_ANDROID)
  bool for_restart = true;
#else
  bool for_restart = false;
#endif

  if (ShouldShowWidevineOptIn() && !is_permission_requested_) {
    is_permission_requested_ = true;
    RequestWidevinePermission(web_contents(), for_restart);
  }
}

void DrmTabHelper::OnEvent(const update_client::CrxUpdateItem& item) {
#if !BUILDFLAG(IS_ANDROID)
  if (item.state == update_client::ComponentState::kUpdated &&
      item.id == kWidevineComponentId) {
#if BUILDFLAG(IS_LINUX)
    if (is_widevine_requested_) {
      RequestWidevinePermission(web_contents(), true /* for_restart*/);
    }
#else
    if (is_widevine_requested_) {
      ReloadIfActive(web_contents());
    }
#endif  // BUILDFLAG(IS_LINUX)
    // Stop observing component update event.
    observer_.Reset();
  }
#endif  // !BUILDFLAG(IS_ANDROID)
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(DrmTabHelper);
