// extracted from rzl lib

#ifndef CHROMIUM_STRING_UTILS_H
#define CHROMIUM_STRING_UTILS_H

#include <string>

namespace machine_id {

bool IsAscii(unsigned char letter);

bool BytesToString(const unsigned char* data,
                   int data_len,
                   std::string* string);

bool GetHexValue(char letter, int* value);

int HexStringToInteger(const char* text);

}  // namespace machine_id
#endif  // CHROMIUM_STRING_UTILS_H
