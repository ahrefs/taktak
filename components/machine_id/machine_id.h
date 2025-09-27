#ifndef CHROMIUM_MACHINE_ID_H
#define CHROMIUM_MACHINE_ID_H

#include <string>

namespace machine_id {

bool GetMachineId(std::string* machine_id);

bool GetRawMachineId(std::u16string* data, int* more_data);

namespace testing {
bool GetMachineIdImpl(const std::u16string& sid_string,
                      int volume_id,
                      std::string* machine_id);
}

}  // namespace machine_id

#endif  // CHROMIUM_MACHINE_ID_H
