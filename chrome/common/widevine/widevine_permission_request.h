#ifndef CHROMIUM_SRC_CHROME_COMMON_WIDEVINE_WIDEVINE_PERMISSION_REQUEST_H_
#define CHROMIUM_SRC_CHROME_COMMON_WIDEVINE_WIDEVINE_PERMISSION_REQUEST_H_

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "components/permissions/permission_request.h"
#include "url/gurl.h"

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
                         bool is_final_decision);
  void DeleteRequest();

  // It's safe to use this raw |web_contents_| because this request is deleted
  // by PermissionManager that is tied with this |web_contents_|.
  raw_ptr<content::WebContents> web_contents_ = nullptr;

  // Only can be true on linux.
  // On linux, browser will use another permission request buble after finishing
  // installation to ask user about restarting because installed widevine can
  // only be used after re-launch.
  bool for_restart_ = false;

  void set_dont_ask_again(bool dont_ask_again) {
    dont_ask_again_ = dont_ask_again;
  }
  bool get_dont_ask_again() const { return dont_ask_again_; }

  bool dont_ask_again_ = false;

  base::WeakPtrFactory<PermissionRequest> weak_factory_{this};
};
#endif  // CHROMIUM_SRC_CHROME_COMMON_WIDEVINE_WIDEVINE_PERMISSION_REQUEST_H_
