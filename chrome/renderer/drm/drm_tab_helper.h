// Copyright (c) 2025 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef CHROMIUM_SRC_CHROME_RENDERER_DRM_DRM_TAB_HELPER_H_
#define CHROMIUM_SRC_CHROME_RENDERER_DRM_DRM_TAB_HELPER_H_

#include "base/scoped_observation.h"
#include "components/drm/taktak_drm.mojom.h"
#include "components/component_updater/component_updater_service.h"
#include "content/public/browser/render_frame_host_receiver_set.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class DrmTabHelper final
    : public content::WebContentsObserver,
      public content::WebContentsUserData<DrmTabHelper>,
      public taktak_drm::mojom::TaktakDRM,
      public component_updater::ComponentUpdateService::Observer {
 public:
  explicit DrmTabHelper(content::WebContents* contents);
  ~DrmTabHelper() override;

  static void BindTaktakDRM(
      mojo::PendingAssociatedReceiver<taktak_drm::mojom::TaktakDRM> receiver,
      content::RenderFrameHost* rfh);

  bool ShouldShowWidevineOptIn() const;

  // content::WebContentsObserver
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;

  // blink::mojom::TaktakDRM
  void HandleWidevineKeySystemRequest() override;

  // component_updater::ComponentUpdateService::Observer
  void OnEvent(const update_client::CrxUpdateItem& item) override;

  WEB_CONTENTS_USER_DATA_KEY_DECL();

 private:
  content::RenderFrameHostReceiverSet<taktak_drm::mojom::TaktakDRM>
      taktak_drm_receivers_;

  bool is_permission_requested_ = false;

  bool is_widevine_requested_ = false;

  base::ScopedObservation<component_updater::ComponentUpdateService,
                          component_updater::ComponentUpdateService::Observer>
      observer_{this};
};
#endif //CHROMIUM_SRC_CHROME_RENDERER_DRM_DRM_TAB_HELPER_H_
