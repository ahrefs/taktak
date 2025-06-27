// Copyright (c) 2024 The Taktak Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 1.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/1.0/.

#ifndef CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_WIDEVINE_UTILS_H_
#define CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_WIDEVINE_UTILS_H_

#include <vector>

#include "base/memory/raw_ptr.h"

namespace permissions {
class PermissionRequest;

bool HasWidevinePermissionRequest(
    const std::vector<
        raw_ptr<permissions::PermissionRequest, VectorExperimental>>& requests);

}  // namespace permissions
#endif  // CHROMIUM_SRC_COMPONENTS_PERMISSIONS_PERMISSION_WIDEVINE_UTILS_H_
