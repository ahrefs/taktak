// Copyright (c) 2024 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 1.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/1.0/.

#include "permission_widevine_utils.h"

#include "components/permissions/permission_request.h"
#include "components/permissions/request_type.h"

namespace permissions {

bool HasWidevinePermissionRequest(
    const std::vector<std::unique_ptr<permissions::PermissionRequest>>&
        requests) {
  if (requests.size() == 1 &&
      requests[0]->request_type() == permissions::RequestType::kWidevine) {
    return true;
  }

  return false;
}

}  // namespace permissions
