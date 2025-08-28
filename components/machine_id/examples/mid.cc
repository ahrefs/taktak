#include "components/machine_id/machine_id.h"

int main() {
  std::string machine_id;
  if (!machine_id::GetMachineId(&machine_id)) {
    return 1;
  }

  printf("%s\n", machine_id.c_str());
}
