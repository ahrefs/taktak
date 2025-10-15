// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef CHROME_BROWSER_MAIN_EXTRA_PARTS_TRACKING_H
#define CHROME_BROWSER_MAIN_EXTRA_PARTS_TRACKING_H

#include <memory>

#include "chrome/browser/chrome_browser_main_extra_parts.h"
#include "chrome/browser/cs/cs_handler.h"
#include "components/web_request_helper/web_request_helper.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace taktak_run_tracking {

class ChromeBrowserMainExtraPartsTracking : public ChromeBrowserMainExtraParts {
 public:
  ChromeBrowserMainExtraPartsTracking();
  ChromeBrowserMainExtraPartsTracking(
      const ChromeBrowserMainExtraPartsTracking&) = delete;
  ChromeBrowserMainExtraPartsTracking& operator=(
      const ChromeBrowserMainExtraPartsTracking&) = delete;
  ~ChromeBrowserMainExtraPartsTracking() override;

  // ChromeBrowserMainExtraParts:
  void PostProfileInit(Profile* profile, bool is_initial_profile) override;

 private:
  void OnTrackOpenTodayEvent(const std::string today_date,
                             web_request_helper::WebRequestResult result);
  void OnTrackFirstRunEvent(web_request_helper::WebRequestResult result);
  void OnTrackOpenWithinSevenDaysEvent(
      web_request_helper::WebRequestResult result);
  std::unique_ptr<cs_handler::CSHandler> cs_handler_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
};

}  // namespace taktak_run_tracking
#endif  // CHROME_BROWSER_MAIN_EXTRA_PARTS_TRACKING_H
