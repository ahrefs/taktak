#include "permission_widevine_utils.h"

#include "components/permissions/permission_request.h"
#include "components/permissions/request_type.h"

namespace permissions {

bool HasWidevinePermissionRequest(
    const std::vector<raw_ptr<permissions::PermissionRequest,
                              VectorExperimental>>& requests) {
  if (requests.size() == 1 &&
      requests[0]->request_type() == permissions::RequestType::kWidevine) {
    return true;
  }

  return false;
}

}  // namespace permissions
