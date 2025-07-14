// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef CHROMIUM_SRC_CHROME_COMMON_WIDEVINE_WIDEVINE_PERMISSION_REQUEST_H_
#define CHROMIUM_SRC_CHROME_COMMON_WIDEVINE_WIDEVINE_PERMISSION_REQUEST_H_

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "components/permissions/permission_request.h"
#include "url/gurl.h"
#include "components/permissions/permission_request_data.h"

namespace content {
class WebContents;
}

class WidevinePermissionRequest : public permissions::PermissionRequest {
 public:
  WidevinePermissionRequest(content::WebContents* web_contents,
                            bool for_restart);

  WidevinePermissionRequest(const WidevinePermissionRequest&) = delete;
  WidevinePermissionRequest& operator=(const WidevinePermissionRequest&) =
      delete;

  ~WidevinePermissionRequest() override;

  std::u16string GetExplanatoryMessageText() const;

  // PermissionRequest overrides:
#if BUILDFLAG(IS_ANDROID)
  PermissionRequest::AnnotatedMessageText GetDialogAnnotatedMessageText(
      const GURL& embedding_origin) const override;
#else
  std::u16string GetMessageTextFragment() const override;
#endif
  void PermissionDecided(ContentSetting result,
                         bool is_one_time,
                         bool is_final_decision,
                         const permissions::PermissionRequestData& request_data);
  void DeleteRequest();

  raw_ptr<content::WebContents> web_contents_ = nullptr;

  // This flag is only applicable on Linux systems. After Widevine installation
  // completes on Linux, the browser displays an additional permission request
  // bubble prompting the user to restart, since the installed Widevine
  // component requires a browser restart to become functional.
  bool for_restart_ = false;

  void set_dont_ask_again(bool dont_ask_again) {
    dont_ask_again_ = dont_ask_again;
  }
  bool get_dont_ask_again() const { return dont_ask_again_; }

  bool dont_ask_again_ = false;

  base::WeakPtrFactory<PermissionRequest> weak_factory_{this};
};
#endif  // CHROMIUM_SRC_CHROME_COMMON_WIDEVINE_WIDEVINE_PERMISSION_REQUEST_H_
