#include "vm.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "number of argument is wrong\n");
    return 1;
  }
  // // or read from serial
  ExecResult result = tarto_vm_run(argv[1]);
  if (result.type != SUCCESS) {
    fprintf(stderr, "runtime error");
    return 1;
  }

  Value val = result.return_value;
  if (val.type == VAL_BOOL) {
    return val.as.boolean;
  }
  return val.as.number;
}
