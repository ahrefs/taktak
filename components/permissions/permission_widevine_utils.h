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
