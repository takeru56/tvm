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

  fp = fopen("test.ir", "r");
  fscanf(fp, "%s", ir);

  ExecResult result = tarto_vm_run(ir);
  if (result.type != SUCCESS) {
    printf("error code: %d\n", result.type);
    fprintf(stderr, "runtime error\n");
    return 1;
  }

  fclose(fp);
  Value val = result.return_value;
  switch(val.type) {
    case VAL_BOOL: {
      return val.as.boolean;
      break;
    }
    case VAL_NUMBER: {
      return val.as.number;
      break;
    }
    case VAL_INSTANCE: {
      return val.as.instance.index;
      break;
    }
    default: {
      return 1;
    }
  }
  return 1;
}
