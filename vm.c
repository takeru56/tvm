#include "vm.h"

Frame *current_frame()
{
  return &vm.frames[vm.frame_index-1];
}

Frame pop_frame()
{
  vm.stack_top = current_frame()->bp;
  vm.frame_index--;
  return vm.frames[vm.frame_index];
}

void push_frame(Frame frame)
{
  frame.bp = vm.stack_top-frame.arg_num;
  vm.frames[vm.frame_index] = frame;
  vm.frame_index++;
}


Frame new_frame(uint16_t ins_size, uint8_t* ins, uint8_t arg_num)
{
  Frame frame = {ins_size, ins, -1};
  frame.arg_num =  arg_num;
  return frame;
}

void vm_init(Bytecode b)
{
  vm.stack_top = vm.stack;
  Frame main = new_frame(b.instruction_size, b.instructions, 0);
  main.bp = vm.stack_top;
  vm.frames[0] = main;
  vm.frame_index = 1;
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
  vm_init(b);

  while(current_frame()->ip < current_frame()->instruction_size - 1) {
    current_frame()->ip++;
    uint8_t* ins = current_frame()->instructions;

    uint16_t ip = current_frame()->ip;
    uint8_t op = ins[ip];

    switch(op) {
      case OP_CONSTANT: {
        uint8_t upper = ins[ip+1];
        uint8_t lower = ins[ip+2];
        current_frame()->ip += 2;
        uint16_t constant_index = decode_constant(upper, lower);
        Constant c = b.constants[constant_index-1];
        switch(c.type) {
          case CONST_INT: {
            upper = c.content[0];
            lower = c.content[1];
            vm_push(NUMBER_VAL(decode_constant(upper, lower)));
            break;
          }
          case CONST_FUNC: {
            vm_push(FUNCTION_VAL(c));
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
        break;
      }
      case OP_LOAD_GLOBAL: {
        uint8_t index = ins[++current_frame()->ip];
        vm_push(vm.global[index]);
        break;
      }
      case OP_STORE_GLOBAL: {
        current_frame()->ip++;
        uint8_t index = ins[current_frame()->ip];
        vm.global[index] = vm_pop();
        break;
      }
      case OP_JNT: {
        Value condition = vm_pop();
        uint8_t upper = ins[++current_frame()->ip];
        uint8_t lower = ins[++current_frame()->ip];
        uint16_t jmp_offset = decode_constant(upper, lower);
        if (condition.as.boolean) {
          break;
        }
        current_frame()->ip = jmp_offset-1;
        break;
      }
      case OP_JMP: {
        uint8_t upper = ins[++current_frame()->ip];
        uint8_t lower = ins[++current_frame()->ip];
        uint16_t jmp_offset = decode_constant(upper, lower);
        current_frame()->ip = jmp_offset-1;
        break;
      }
      case OP_CALL: {
        int8_t arg_num = ins[++current_frame()->ip];
        Value constant = *(vm.stack_top-arg_num-1);

        if (constant.as.function.type != CONST_FUNC) return EXEC_RESULT(ERROR_OTHER, NIL_VAL());
        push_frame(new_frame(constant.as.function.size, constant.as.function.content, arg_num));
        break;
      }
      case OP_RETURN: {
        Value val = vm_pop();
        pop_frame();
        vm_pop(); // pop function
        vm_push(val);
        break;
      }
      case OP_LOAD_LOCAL: {
        uint8_t index = ins[++current_frame()->ip];
        if (index < current_frame()->arg_num) {
          Value arg_val = current_frame()->bp[index];
          vm_push(arg_val);
          break;
        }
        vm_push(current_frame()->local[index]);
        break;
      }
      case OP_STORE_LOCAL: {
        uint8_t index = ins[++current_frame()->ip];
        if (index < current_frame()->arg_num) {
          current_frame()->bp[index] = vm_pop();
          break;
        }
        current_frame()->local[index] = vm_pop();
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
  Bytecode bcode;
  uint8_t *content;
  uint8_t* insts = calloc(sizeof(uint8_t), INST_MAX);
  Constant* constants = calloc(sizeof(Constant), CONST_MAX);
  int p = 0;
  int cnt = 0;
  int pos = 0;


  // u4 skip magic number
  for (int i=0; i<4; i++) {
    cnt += 2;
    pos++;
  }

  // u1 parse class_pool_count
  uint8_t up =  str[cnt++];
  uint8_t low = str[cnt++];
  uint8_t class_pool_size = calc_byte(up, low);

  // parse class pool
  for (int i=0; i<class_pool_size; i++) {
    up =  str[cnt++];
    low = str[cnt++];
    uint8_t upper = calc_byte(up, low);
    pos++;
    up =  str[cnt++];
    low = str[cnt++];
    pos++;
    uint8_t lower= calc_byte(up, low);
    uint8_t class_constant_pool_size = decode_constant(upper, lower);
    Constant* class_constants = calloc(sizeof(Constant), CONST_MAX);

    // parse class constant pool
    for (int j=0; j<class_constant_pool_size; j++) {
      up =  str[cnt++];
      low = str[cnt++];
      pos++;
      uint8_t class_const_type = calc_byte(up, low);
      up =  str[cnt++];
      low = str[cnt++];
      pos++;
      upper = calc_byte(up, low);
      up =  str[cnt++];
      low = str[cnt++];
      pos++;
      lower = calc_byte(up, low);
      uint16_t class_const_size = decode_constant(upper, lower);
      switch (class_const_type) {
        case CONST_INT: {
          content = calloc(sizeof(uint8_t), 2);
          for (int k=0; k<class_const_size; k++) {
            up =  str[cnt++];
            low = str[cnt++];
            pos++;
            content[k] = calc_byte(up, low);
          }
          break;
        }
        case CONST_FUNC: {
          content = calloc(sizeof(uint8_t), INST_MAX);
          for (int k=0; k<class_const_size; k++) {
            up =  str[cnt++];
            low = str[cnt++];
            pos++;
            content[k] = calc_byte(up, low);
          }
          break;
        }
      }
      class_constants[j].type = class_const_type;
      class_constants[j].size = class_const_size;
      class_constants[j].content = content;
    }
    bcode.classes[i].constant_size = class_constant_pool_size;
    bcode.classes[i].constants = class_constants;
  }

  // parse constant_pool_count
  up =  str[cnt++];
  low = str[cnt++];
  uint8_t upper = calc_byte(up, low);
  pos++;
  up =  str[cnt++];
  low = str[cnt++];
  pos++;
  uint8_t lower= calc_byte(up, low);
  uint8_t constant_pool_size = decode_constant(upper, lower);

  // parse content
  for (int i=0; i<constant_pool_size; i++) {
    up =  str[cnt++];
    low = str[cnt++];
    pos++;
    uint8_t const_type = calc_byte(up, low);
    up =  str[cnt++];
    low = str[cnt++];
    pos++;
    upper = calc_byte(up, low);
    up =  str[cnt++];
    low = str[cnt++];
    pos++;
    lower = calc_byte(up, low);
    uint16_t const_size = decode_constant(upper, lower);
    switch (const_type) {
      case CONST_INT: {
        content = calloc(sizeof(uint8_t), 2);
        for (int j=0; j<const_size; j++) {
          up =  str[cnt++];
          low = str[cnt++];
          pos++;
          content[j] = calc_byte(up, low);
        }
        break;
      }
      case CONST_FUNC: {
        content = calloc(sizeof(uint8_t), INST_MAX);
        for (int j=0; j<const_size; j++) {
          up =  str[cnt++];
          low = str[cnt++];
          pos++;
          content[j] = calc_byte(up, low);
        }
        break;
      }
    }
    constants[i].type = const_type;
    constants[i].size = const_size;
    constants[i].content = content;
  }

  // parse instructions
  up =  str[cnt++];
  low = str[cnt++];
  pos++;
  upper = calc_byte(up, low);
  up =  str[cnt++];
  low = str[cnt++];
  pos++;
  lower = calc_byte(up, low);
  uint16_t inst_size = decode_constant(upper, lower);
  // parse content
  for (int i=0; i<inst_size; i++) {
    uint8_t up =  str[cnt++];
    uint8_t low = str[cnt++];
    pos++;
    insts[i] = calc_byte(up, low);
  }

  bcode.instruction_size = inst_size;
  bcode.instructions = insts;
  bcode.constants = constants;
  bcode.constant_size = constant_pool_size;
  return bcode;
}

ExecResult tarto_vm_run(char* input)
{
  Bytecode bytecode = parse_bytecode(input);

  // for debug
  // printf("** instruction**\n");
  // for (int i=0; i<bytecode.instruction_size; i++) {
  //   printf("%d: %d\n", i, bytecode.instructions[i]);
  // }
  ExecResult er = exec_interpret(bytecode);
  free(bytecode.constants);
  free(bytecode.instructions);
  return er;
}
