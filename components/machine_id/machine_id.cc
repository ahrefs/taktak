#include "machine_id.h"

#include <stddef.h>

#include <algorithm>
#include <fstream>

#include "assert.h"
#include "base/hash/sha1.h"
#include "base/rand_util.h"
#include "base/strings/stringprintf.h"
#include "build/chromeos_buildflags.h"
#include "crc8.h"
#include "string_utils.h"

namespace machine_id {

bool GetMachineId(std::string* machine_id) {
  if (!machine_id) {
    return false;
  }

#if BUILDFLAG(IS_LINUX)
  std::ifstream file("/etc/machine-id");
  std::string mid;
  if (file.is_open()) {
    std::getline(file, mid);
    file.close();
  }
  *machine_id = mid;
  return !mid.empty();
#else

  static std::string calculated_id;
  static bool calculated = false;
  if (calculated) {
    *machine_id = calculated_id;
    return true;
  }

  std::u16string sid_string;
  int volume_id;
  if (!GetRawMachineId(&sid_string, &volume_id)) {
    return false;
  }

  if (!testing::GetMachineIdImpl(sid_string, volume_id, machine_id)) {
    return false;
  }

  calculated = true;
  calculated_id = *machine_id;
  return true;

#endif  // BUILDFLAG(IS_LINUX)
}

namespace testing {

bool GetMachineIdImpl(const std::u16string& sid_string,
                      int volume_id,
                      std::string* machine_id) {
  machine_id->clear();

  // The ID should be the SID hash + the Hard Drive SNo. + checksum byte.
  static const int kSizeWithoutChecksum = base::kSHA1Length + sizeof(int);
  std::vector<unsigned char> id_binary(kSizeWithoutChecksum + 1, 0);

  if (!sid_string.empty()) {
    size_t byte_count = sid_string.size() * sizeof(std::u16string::value_type);
    const char* buffer = reinterpret_cast<const char*>(sid_string.c_str());
    std::string sid_string_buffer(buffer, byte_count);

    std::string digest(base::SHA1HashString(sid_string_buffer));
    VERIFY(digest.size() == base::kSHA1Length);
    std::ranges::copy(digest, id_binary.begin());
  }

  // Convert from int to binary (makes big-endian).
  for (size_t i = 0; i < sizeof(int); i++) {
    int shift_bits = 8 * (sizeof(int) - i - 1);
    id_binary[base::kSHA1Length + i] =
        static_cast<unsigned char>((volume_id >> shift_bits) & 0xFF);
  }

  // Append the checksum byte.
  if (!sid_string.empty() || (0 != volume_id)) {
    Crc8::Generate(id_binary.data(), kSizeWithoutChecksum,
                   &id_binary[kSizeWithoutChecksum]);
  }

  return BytesToString(id_binary.data(), kSizeWithoutChecksum + 1, machine_id);
}

}  // namespace testing

}  // namespace machine_id
