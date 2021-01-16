#include "vm.h"

void vm_init()
{
  vm = (typeof(vm)){NULL};
  vm.stack_top = vm.stack;
}

void vm_push(Value value)
{
  *vm.stack_top = value;
  vm.stack_top++;
}

Value vm_pop()
{
  vm.stack_top--;
  return *vm.stack_top;
}

uint8_t trans(unsigned char c)
{
	if('0'<=c && c<='9') return (c-0x30);
	if('A'<=c && c<='F') return (c+0x0A-0x41);
	if('a'<=c && c<='f') return (c+0x0A-0x61);
	return 0;
}

uint8_t calc_byte(unsigned char upper, unsigned char lower) {
  uint8_t up =  16*trans(upper);
  uint8_t low = trans(lower);
  return up + low;
}

uint16_t decode_constant(uint8_t upper, uint8_t lower)
{
  // big endian
  return (255*upper + lower);
}

ExecResult exec_interpret(Bytecode b)
{
  vm_init();

  vm.bp = b.instructions;
  vm.ip = vm.bp;
  bool f = true;
  while(f) {
    uint8_t instruction = *vm.ip++;
    switch(instruction) {
      case OP_CONSTANT: {
        uint8_t upper = *vm.ip++;
        uint8_t lower = *vm.ip++;
        uint16_t constant_index = decode_constant(upper, lower);
        Constant c = b.constants[constant_index-1];
        switch(c.type) {
          case CONST_INT: {
            upper = *c.content++;
            lower = *c.content;
            uint16_t val = decode_constant(upper, lower);
            vm_push(NUMBER_VAL(val));
            break;
          }
          case CONST_FUNC: {
            vm_push(FUNCTION_VAL(&c));
            break;
          }
        }
        break;
      }
      case OP_ADD: {
        Value r = vm_pop();
        Value l = vm_pop();
        vm_push(NUMBER_VAL(l.as.number+r.as.number));
        break;
      }
      case OP_SUB: {
        Value r = vm_pop();
        Value l = vm_pop();
        vm_push(NUMBER_VAL(l.as.number-r.as.number));
        break;
      }
      case OP_MUL: {
        Value r = vm_pop();
        Value l = vm_pop();
        vm_push(NUMBER_VAL(l.as.number*r.as.number));
        break;
      }
      case OP_DIV: {
        Value r = vm_pop();
        Value l = vm_pop();
        if (r.as.number == 0) {
          return EXEC_RESULT(ERROR_DIVISION_BY_ZERO, NIL_VAL());
        }
        vm_push(NUMBER_VAL(l.as.number/r.as.number));
        break;
      }
      case OP_EQ: {
        Value r = vm_pop();
        Value l = vm_pop();
        if (l.as.number-r.as.number == 0) {
          vm_push(BOOL_VAL(true));
          break;
        }
        vm_push(BOOL_VAL(false));
        break;
      }
      case OP_NEQ: {
        Value r = vm_pop();
        Value l = vm_pop();
        if (l.as.number-r.as.number == 0) {
          vm_push(BOOL_VAL(false));
          break;
        }
        vm_push(BOOL_VAL(true));
        break;
      }
      case OP_LESS: {
        Value r = vm_pop();
        Value l = vm_pop();
        if (r.as.number-l.as.number > 0) {
          vm_push(BOOL_VAL(true));
          break;
        }
        vm_push(BOOL_VAL(false));
        break;
      }
      case OP_GREATER: {
        Value r = vm_pop();
        Value l = vm_pop();
        if (l.as.number-r.as.number > 0) {
          vm_push(BOOL_VAL(true));
          break;
        }
        vm_push(BOOL_VAL(false));
        break;
      }
      case OP_DONE: {
        f = false;
        break;
      }
      case OP_LOAD_GLOBAL: {
        uint8_t index = *vm.ip++;
        vm_push(vm.global[index]);
        break;
      }
      case OP_STORE_GLOBAL: {
        uint8_t index = *vm.ip++;
        vm.global[index] = vm_pop();
        vm_push(vm.global[index]);
        break;
      }
      case OP_JNT: {
        Value condition = vm_pop();
        uint8_t upper = *vm.ip++;
        uint8_t lower = *vm.ip++;
        uint16_t jmp_offset = decode_constant(upper, lower);
        if (condition.as.boolean) {
          break;
        }
        vm.ip = vm.bp + jmp_offset;
        break;
      }
      case OP_JMP: {
        uint8_t upper = *vm.ip++;
        uint8_t lower = *vm.ip++;
        uint16_t jmp_offset = decode_constant(upper, lower);
        vm.ip = vm.bp + jmp_offset;
        break;
      }
      default:
        return EXEC_RESULT(ERROR_UNKNOWN_OPCODE, NIL_VAL());
    }
  }

  Value val = vm_pop();
  return EXEC_RESULT(SUCCESS, val);
}

Bytecode parse_bytecode(char* str)
{
  Bytecode b;
  Constant* constants = (Constant *)malloc(CONST_MAX);
  uint8_t*  instructions = (uint8_t *)malloc(INST_MAX);
  // if(!bytecode) {
  // }
  // if(!constants) {
  // }
  int p = 0, cnt = 0, pos = 0;
  uint16_t constant_pool_size = 0;

  // parse constants
  while (cnt < strlen(str)){
    uint8_t up =  str[cnt++];
    uint8_t low = str[cnt++];
    uint8_t b = calc_byte(up, low);
    pos++;

    // skip magic number
    if (pos <= 4) continue;
    // parse constant_pool_count
    if (pos == 5) {
      up =  str[cnt++];
      low = str[cnt++];
      pos++;
      constant_pool_size = decode_constant(b, calc_byte(up, low));
    }
    // parse content
    for (int i=0; i<constant_pool_size; i++) {
      up =  str[cnt++];
      low = str[cnt++];
      pos++;
      uint8_t const_type = calc_byte(up, low);
      up =  str[cnt++];
      low = str[cnt++];
      pos++;
      uint8_t upper = calc_byte(up, low);
      up =  str[cnt++];
      low = str[cnt++];
      pos++;
      uint8_t lower = calc_byte(up, low);
      uint16_t const_size = decode_constant(upper, lower);

      switch (const_type) {
        case CONST_INT: {
          uint8_t* content = (uint8_t *)malloc(2);
          for (int j=0; j<const_size; j++) {
            up =  str[cnt++];
            low = str[cnt++];
            pos++;
            content[j] = calc_byte(up, low);
          }
          constants[i].type = const_type;
          constants[i].size = const_size;
          constants[i].content = content;
          break;
        }
        case CONST_FUNC: {
          uint8_t* content = (uint8_t *)malloc(INST_MAX);
          for (int j=0; j<const_size; j++) {
            up =  str[cnt++];
            low = str[cnt++];
            pos++;
            content[j] = calc_byte(up, low);
          }
          constants[i].type = const_type;
          constants[i].size = const_size;
          constants[i].content = content;
          break;
        }
      }
    }
    break;
  }
  b.constants = constants;
  b.constant_size = constant_pool_size;

  // parse instructions
  uint8_t up =  str[cnt++];
  uint8_t low = str[cnt++];
  pos++;
  uint8_t upper = calc_byte(up, low);
  up =  str[cnt++];
  low = str[cnt++];
  pos++;
  uint8_t lower = calc_byte(up, low);
  uint16_t inst_size = decode_constant(upper, lower);
  // parse content
  for (int i=0; i<inst_size; i++) {
    uint8_t up =  str[cnt++];
    uint8_t low = str[cnt++];
    pos++;
    instructions[i] = calc_byte(up, low);
  }
  b.instruction_size = inst_size;
  b.instructions = instructions;

  return b;
}

ExecResult tarto_vm_run(char* input)
{
  Bytecode bytecode = parse_bytecode(input);

  // printf("const size: %d\n", bytecode.constant_size);
  // printf("instruction\n");
  // for (int i=0; i<bytecode.instruction_size; i++) {
  //   printf("%d: %d\n", i, bytecode.instructions[i]);
  // }
  return exec_interpret(bytecode);
}
