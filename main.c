#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define STACK_MAX 256
#define GLOBAL_MAX 256

struct {
  uint8_t *ip;
  uint16_t stack[STACK_MAX];
  uint16_t global[GLOBAL_MAX];
  uint16_t *stack_top;
} vm;

typedef enum {
  OP_CONSTANT,
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_DONE,
  LOAD_GLOBAL,
  STORE_GLOBAL,
} opcode;

typedef enum exec_result {
  SUCCESS,
  ERROR_DIVISION_BY_ZERO,
  ERROR_UNKNOWN_OPCODE,
} exec_result;

void vm_init(){
  vm = (typeof(vm)){NULL};
  vm.stack_top = vm.stack;
}

void vm_push(uint16_t value) {
  *vm.stack_top = value;
  vm.stack_top++;
}

uint16_t vm_pop() {
  vm.stack_top--;
  return *vm.stack_top;
}

uint8_t trans(unsigned char c){
	if('0'<=c && c<='9') return (c-0x30);
	if('A'<=c && c<='F') return (c+0x0A-0x41);
	if('a'<=c && c<='f') return (c+0x0A-0x61);
	return 0;
}

uint16_t decode_constant(uint8_t upper, uint8_t lower) {
  // bigendian
  return 255*upper + lower;
}

exec_result exec_interpret(uint8_t *bytecode) {
  vm_init();

  vm.ip = bytecode;
  while(1) {
    uint8_t instruction = *vm.ip++;
    switch(instruction) {
      case OP_CONSTANT: {
        uint8_t upper = *vm.ip++;
        uint8_t lower = *vm.ip++;
        uint16_t constant = decode_constant(upper, lower);
        vm_push(constant);
        break;
      }
      case OP_ADD: {
        uint16_t r = vm_pop();
        uint16_t l = vm_pop();
        vm_push(l+r);
        break;
      }
      case OP_SUB: {
        uint16_t r = vm_pop();
        uint16_t l = vm_pop();
        vm_push(l-r);
        break;
      }
      case OP_MUL: {
        uint16_t r = vm_pop();
        uint16_t l = vm_pop();
        vm_push(l*r);
        break;
      }
      case OP_DIV: {
        uint16_t r = vm_pop();
        uint16_t l = vm_pop();
        if (r == 0) {
          return ERROR_DIVISION_BY_ZERO;
        }
        vm_push(l/r);
        break;
      }
      case OP_DONE: return SUCCESS;
      case LOAD_GLOBAL: {
        uint8_t index = *vm.ip++;
        vm_push(vm.global[index]);
        break;
      }
      case STORE_GLOBAL: {
        uint8_t index= *vm.ip++;
        vm.global[index] = vm_pop();
        vm_push(vm.global[index]);
        break;
      }
      default:
      return ERROR_UNKNOWN_OPCODE;
    }
  }
  return SUCCESS;
}

void debug(uint8_t* pointer, int cap) {
  int cnt = 0;
  printf("%s\n", "== this is debug info ==");
  printf("word size: %lu\n", sizeof(pointer));
  while (cnt < cap){
    int idx = cnt;
    printf("[%d]: %d\n", idx,  pointer[cnt++]);
  }
  printf("%s\n", "========================");
}

uint8_t* parse_bytecode(char* str) {
  uint8_t* bytecode;
  // TODO: replace 100
  bytecode = (uint8_t *)malloc(100);
  if(!bytecode) {
  }
  int p = 0, cnt = 0;
  while (cnt < strlen(str)){
    uint8_t up =  16*trans(str[cnt++]);
    uint8_t low = trans(str[cnt++]);
    bytecode[p++] = up+low;
  }
  return bytecode;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "number of argument is wrong\n");
    return 1;
  }
  uint8_t* bytecode;
  bytecode = parse_bytecode(argv[1]);
  // or read from serial
  exec_result res = exec_interpret(bytecode);
  if (res != SUCCESS) {
    fprintf(stderr, "runtime error");
    return 1;
  }
  free(bytecode);
  return vm_pop();
}
