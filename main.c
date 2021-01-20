#include "vm.h"

int main(int argc, char **argv) {
  // 引数から
  // if (argc != 2) {
  //   fprintf(stderr, "number of argument is wrong\n");
  //   return 1;
  // }
  // ExecResult result = tarto_vm_run(argv[1]);
  char ir[IR_MAX];
  FILE *fp;

  fp = fopen("out.tt", "r");
  fscanf(fp, "%s", ir);

  ExecResult result = tarto_vm_run(ir);
  if (result.type != SUCCESS) {
    printf("error code: %d\n", result.type);
    fprintf(stderr, "runtime error\n");
    return 1;
  }

  fclose(fp);
  Value val = result.return_value;
  if (val.type == VAL_BOOL) {
    return val.as.boolean;
  }
  if (val.type == VAL_NUMBER) {
    return val.as.number;
  }
  return 1;
}
