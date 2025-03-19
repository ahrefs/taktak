#include <string>

#include "base/ios/device_util.h"
#include "base/strings/utf_string_conversions.h"

namespace machine_id {

bool GetRawMachineId(std::u16string* data, int* more_data) {
  *data = base::ASCIIToUTF16(ios::device_util::GetDeviceIdentifier(NULL));
  *more_data = 1;
  return true;
}

}  // namespace machine_id
