// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "widevine_permission_request.h"

#include "build/build_config.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/grit/generated_resources.h"
#include "components/permissions/permission_widevine_utils.h"
#include "components/permissions/request_type.h"
#include "components/permissions/resolvers/content_setting_permission_resolver.h"
#include "components/prefs/pref_service.h"
#include "components/url_formatter/elide_url.h"
#include "components/vector_icons/vector_icons.h"
#include "constants.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "widevine_utils.h"

WidevinePermissionRequest::WidevinePermissionRequest(
    content::WebContents* web_contents,
    bool for_restart)
    : PermissionRequest(
          std::make_unique<permissions::PermissionRequestData>(
              std::make_unique<permissions::ContentSettingPermissionResolver>(
                  permissions::RequestType::kWidevine),
              false,
              web_contents->GetVisibleURL()),
          base::BindRepeating(&WidevinePermissionRequest::PermissionDecided,
                              base::Unretained(this))),
      web_contents_(web_contents),
      for_restart_(for_restart) {}

WidevinePermissionRequest::~WidevinePermissionRequest() = default;

#if BUILDFLAG(IS_ANDROID)
permissions::PermissionRequest::AnnotatedMessageText
WidevinePermissionRequest::GetDialogAnnotatedMessageText(
    const GURL& embedding_origin) const {
  return permissions::PermissionRequest::AnnotatedMessageText(
      l10n_util::GetStringFUTF16(
          GetWidevinePermissionRequestTextFrangmentResourceId(false),
          url_formatter::FormatUrlForSecurityDisplay(
              requesting_origin(),
              url_formatter::SchemeDisplay::OMIT_CRYPTOGRAPHIC)),
      {});
}
#else
std::u16string WidevinePermissionRequest::GetMessageTextFragment() const {
  return l10n_util::GetStringUTF16(
      GetWidevinePermissionRequestTextFrangmentResourceId(for_restart_));
}
#endif

void WidevinePermissionRequest::PermissionDecided(ContentSetting result,
                                                  bool is_one_time,
                                                  bool is_final_decision,
                                                  const permissions::PermissionRequestData& request_data) {
  // Permission granted
  if (result == ContentSetting::CONTENT_SETTING_ALLOW) {
    if (!for_restart_) {
      EnableWidevineCdm();
    } else {
#if BUILDFLAG(IS_ANDROID)
      EnableWidevineCdm();
#endif
      base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
          FROM_HERE, base::BindOnce(&chrome::AttemptRelaunch));
    }
    // Permission denied
  } else if (result == ContentSetting::CONTENT_SETTING_BLOCK) {
    Profile* profile =
        static_cast<Profile*>(web_contents_->GetBrowserContext());
    profile->GetPrefs()->SetBoolean(kAskWidvineInstall, !get_dont_ask_again());
    // Cancelled
  } else {
    DCHECK(result == CONTENT_SETTING_DEFAULT);
    // Do nothing.
  }
}

std::u16string WidevinePermissionRequest::GetExplanatoryMessageText() const {
  return l10n_util::GetStringUTF16(IDS_WIDEVINE_INSTALL_MESSAGE);
}
