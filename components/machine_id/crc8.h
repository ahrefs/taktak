#ifndef CHROMIUM_CRC8_H
#define CHROMIUM_CRC8_H
namespace machine_id {
// CRC-8 methods:
class Crc8 {
 public:
  static bool Generate(const unsigned char* data,
                       int length,
                       unsigned char* check_sum);
  static bool Verify(const unsigned char* data,
                     int length,
                     unsigned char checksum,
                     bool* matches);
};
}  // namespace machine_id
#endif  // CHROMIUM_CRC8_H
