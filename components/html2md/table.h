#ifndef CHROMIUM_TABLE_H
#define CHROMIUM_TABLE_H

#include <string>

namespace html2md_table {
[[nodiscard]] std::string formatMarkdownTable(const std::string& inputTable);
}

#endif  // CHROMIUM_TABLE_H
