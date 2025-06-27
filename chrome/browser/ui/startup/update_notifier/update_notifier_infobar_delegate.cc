// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "update_notifier_infobar_delegate.h"

#include <memory>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/task/single_thread_task_runner.h"
#include "base/types/pass_key.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/infobars/confirm_infobar_creator.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/grit/generated_resources.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "components/infobars/core/infobar.h"
#include "components/omnibox/browser/vector_icons.h"
#include "components/vector_icons/vector_icons.h"
#include "content/public/common/content_switches.h"
#include "ui/base/l10n/l10n_util.h"
#include "chrome/browser/buildflags.h"

// static
infobars::InfoBar* UpdateNotifierInfoBarDelegate::Create(
    infobars::ContentInfoBarManager* infobar_manager,
    Profile* profile) {
  return infobar_manager->AddInfoBar(
      CreateConfirmInfoBar(std::make_unique<UpdateNotifierInfoBarDelegate>(
          base::PassKey<UpdateNotifierInfoBarDelegate>(), profile)));
}

UpdateNotifierInfoBarDelegate::UpdateNotifierInfoBarDelegate(
    base::PassKey<UpdateNotifierInfoBarDelegate>,
    Profile* profile)
    : profile_(profile) {}

UpdateNotifierInfoBarDelegate::~UpdateNotifierInfoBarDelegate() = default;

infobars::InfoBarDelegate::InfoBarIdentifier
UpdateNotifierInfoBarDelegate::GetIdentifier() const {
 return UPDATE_NOTIFIER_INFOBAR_DELEGATE;
}

const gfx::VectorIcon& UpdateNotifierInfoBarDelegate::GetVectorIcon() const {
  return dark_mode() ? omnibox::kProductChromeRefreshIcon
                     : vector_icons::kProductIcon;
}

bool UpdateNotifierInfoBarDelegate::ShouldExpire(
    const NavigationDetails& details) const {
  return false;
}

void UpdateNotifierInfoBarDelegate::InfoBarDismissed() {
  ConfirmInfoBarDelegate::InfoBarDismissed();
}

std::u16string UpdateNotifierInfoBarDelegate::GetMessageText() const {
  return l10n_util::GetStringUTF16(IDS_UPDATE_NOTIFIER_MESSAGE);
}

int UpdateNotifierInfoBarDelegate::GetButtons() const {
  return BUTTON_OK;
}

std::u16string UpdateNotifierInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  DCHECK_EQ(BUTTON_OK, button);
  return l10n_util::GetStringUTF16(IDS_UPDATE_NOTIFIER_BUTTON_TEXT);
}

bool UpdateNotifierInfoBarDelegate::Accept() {
  Browser* browser = chrome::FindLastActiveWithProfile(profile_);
  if (!browser) {
    return ConfirmInfoBarDelegate::Accept();
  }

  content::OpenURLParams params(
      GURL{BUILDFLAG(TAKTAK_DOWNLOAD_URL)}, content::Referrer(),
      WindowOpenDisposition::NEW_FOREGROUND_TAB, ui::PAGE_TRANSITION_LINK,
      false /* is_renderer_initiated */);
  browser->OpenURL(params, /*navigation_handle_callback=*/{});

  return ConfirmInfoBarDelegate::Accept();
}
