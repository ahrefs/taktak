// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_UPDATE_NOTIFIER_UPDATE_NOTIFIER_INFOBAR_DELEGATE_H_
#define CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_UPDATE_NOTIFIER_UPDATE_NOTIFIER_INFOBAR_DELEGATE_H_

#include "base/memory/raw_ptr.h"
#include "base/types/pass_key.h"
#include "chrome/browser/shell_integration.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/confirm_infobar_delegate.h"

class Profile;

// The infobar takes ownership of this delegate and manages its lifetime,
// which is tied to the associated WebContents instance.
class UpdateNotifierInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  static infobars::InfoBar *Create(
      infobars::ContentInfoBarManager *infobar_manager,
      Profile *profile);

  UpdateNotifierInfoBarDelegate(const UpdateNotifierInfoBarDelegate &) = delete;
  UpdateNotifierInfoBarDelegate &operator=(const UpdateNotifierInfoBarDelegate &) = delete;

  UpdateNotifierInfoBarDelegate(base::PassKey<UpdateNotifierInfoBarDelegate>, Profile* profile);
  ~UpdateNotifierInfoBarDelegate() override;

 private:
  // ConfirmInfoBarDelegate:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  const gfx::VectorIcon &GetVectorIcon() const override;
  bool ShouldExpire(const NavigationDetails &details) const override;
  void InfoBarDismissed() override;
  std::u16string GetMessageText() const override;
  int GetButtons() const override;
  std::u16string GetButtonLabel(InfoBarButton button) const override;
  bool Accept() override;

  raw_ptr<Profile> profile_;
};
#endif //CHROMIUM_SRC_CHROME_BROWSER_UI_STARTUP_UPDATE_NOTIFIER_UPDATE_NOTIFIER_INFOBAR_DELEGATE_H_