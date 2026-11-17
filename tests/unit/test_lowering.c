#define SNOW_ENABLED 1
#include <math.h>
#include "helpers/fake_tokens.h"
#include "ir.h"
#include "lowering.h"
#include "semantics.h"
#include "snow/snow.h"

#define MAIN                                                            \
  LINE(0), TOK(AT_MAIN), TOK(DEF), ID("main"), UNIT_VALUE, TOK(ASSIGN), \
      UNIT_VALUE, EOL
#define F_HEAD LINE(0), TOK(DEF), ID("f"), UNIT_VALUE, TOK(ASSIGN)
#define CHECK_LOWER(...)    \
  do {                      \
    PARSE(__VA_ARGS__);     \
    semantics_analyze(ctx); \
    lower_ast(ctx);         \
    validate_ir(ctx);       \
  } while (0)

/* A bounded test-only numeric interpreter checks effects and control flow,
 * rather than requiring a particular block or temporary-slot layout. */
typedef struct {
  int reads, prints, exit_code, budget;
  double input;
} Execution;

static IrOperand operand(IrOperand op, IrOperand* values) {
  return op.type == IR_OP_VALUE ? values[op.value.id] : op;
}

static double number(IrOperand op) {
  if (op.type == IR_OP_BOOL)
    return op.value.bool_value;
  return op.type == IR_OP_DOUBLE ? op.value.double_value : op.value.int32_value;
}

static IrOperand numeric(double value, IrType type) {
  if (type == IR_BOOL)
    return (IrOperand){.type = IR_OP_BOOL, .value.bool_value = value != 0};
  if (type == IR_DOUBLE)
    return (IrOperand){.type = IR_OP_DOUBLE, .value.double_value = value};
  return (IrOperand){.type = IR_OP_INT32, .value.int32_value = (IrInt)value};
}

static IrOperand execute(Ctx* ctx,
                         IrFunctionId id,
                         IrOperand* arguments,
                         Execution* state) {
  IrFunction* fn = &ctx->ir.functions.data[id];
  IrOperand slots[256] = {0}, values[512] = {0};
  assert(fn->slots.count <= 256 && fn->value_count <= 512);
  for (size_t i = 0; i < fn->parameter_count; i++)
    slots[i] = arguments[i];
  IrBlockId block = fn->entrypoint;
  while (state->budget-- > 0) {
    IrBlock* current = &fn->blocks.data[block];
    for (size_t i = 0; i < current->instructions.count; i++) {
      IrInstruction* inst = &current->instructions.data[i];
      IrOperand result = {.type = IR_OP_UNIT};
      if (inst->type == IR_INST_LOAD) {
        result = slots[inst->operands.load.source];
      } else if (inst->type == IR_INST_STORE) {
        slots[inst->operands.store.target] =
            operand(inst->operands.store.source, values);
      } else if (inst->type == IR_INST_CALL) {
        assert(inst->operands.call.arg_count <= 16);
        IrOperand args[16];
        for (size_t j = 0; j < inst->operands.call.arg_count; j++)
          args[j] = operand(inst->operands.call.args[j], values);
        switch (inst->operands.call.type) {
          case IR_FX_USER:
            result = execute(ctx, inst->operands.call.id, args, state);
            break;
          case IR_FX_READ_INT:
          case IR_FX_READ_BOOL:
          case IR_FX_READ_DOUBLE:
            state->reads++;
            result = numeric(state->input, inst->result_type);
            break;
          case IR_FX_PRINT:
            state->prints++;
            break;
          case IR_FX_TO_INT:
            result = numeric(trunc(number(args[0])), IR_INT);
            break;
          default:
            assert(false, "Unsupported test interpreter call");
        }
        if (state->exit_code)
          return result;
      } else if (inst->type == IR_INST_NOT ||
                 inst->type == IR_INST_INT2DOUBLE ||
                 inst->type == IR_INST_FLOOR) {
        double value = number(operand(inst->operands.unary.operand, values));
        result = numeric(inst->type == IR_INST_NOT     ? !value
                         : inst->type == IR_INST_FLOOR ? floor(value)
                                                       : value,
                         inst->result_type);
      } else {
        double left = number(operand(inst->operands.binary.left, values));
        double right = number(operand(inst->operands.binary.right, values));
        double out = 0;
        switch (inst->type) {
          case IR_INST_ADD:
            out = left + right;
            break;
          case IR_INST_SUB:
            out = left - right;
            break;
          case IR_INST_MUL:
            out = left * right;
            break;
          case IR_INST_IDIV:
            out = trunc(left / right);
            break;
          case IR_INST_DIV:
            out = left / right;
            break;
          case IR_INST_EQ:
            out = left == right;
            break;
          case IR_INST_NE:
            out = left != right;
            break;
          case IR_INST_LT:
            out = left < right;
            break;
          case IR_INST_LE:
            out = left <= right;
            break;
          case IR_INST_GT:
            out = left > right;
            break;
          case IR_INST_GE:
            out = left >= right;
            break;
          default:
            assert(false, "Unsupported test interpreter opcode");
        }
        result = numeric(out, inst->result_type);
      }
      if (inst->result != IR_INVALID_ID)
        values[inst->result] = result;
    }
    IrBlockTerminator* term = &current->terminator;
    switch (term->type) {
      case IR_TERMINATOR_RETURN:
        return operand(term->value.return_value, values);
      case IR_TERMINATOR_EXIT:
        state->exit_code = (int)number(operand(term->value.exit_code, values));
        return (IrOperand){.type = IR_OP_UNIT};
      case IR_TERMINATOR_JUMP:
        block = term->value.jump_target;
        break;
      case IR_TERMINATOR_CONDITIONAL:
        block =
            number(operand(term->value.conditional_target.condition, values))
                ? term->value.conditional_target.true_target
                : term->value.conditional_target.false_target;
        break;
    }
  }
  assert(false, "IR failed to terminate within the test execution budget");
  return (IrOperand){.type = IR_OP_UNIT};
}

static void validate_ir(Ctx* ctx) {
  assert(ctx->ir.entrypoint < ctx->ir.functions.count);
  for (size_t f = 0; f < ctx->ir.functions.count; f++) {
    IrFunction* fn = &ctx->ir.functions.data[f];
    assert(fn->entrypoint < fn->blocks.count);
    for (size_t b = 0; b < fn->blocks.count; b++) {
      IrBlock* block = &fn->blocks.data[b];
      IrBlockTerminator* term = &block->terminator;
      if (term->type == IR_TERMINATOR_JUMP)
        assert(term->value.jump_target < fn->blocks.count);
      if (term->type == IR_TERMINATOR_CONDITIONAL) {
        assert(term->value.conditional_target.true_target < fn->blocks.count);
        assert(term->value.conditional_target.false_target < fn->blocks.count);
      }
      for (size_t i = 0; i < block->instructions.count; i++) {
        IrInstruction* inst = &block->instructions.data[i];
        if (inst->result_type == IR_UNIT || inst->type == IR_INST_STORE)
          assert(inst->result == IR_INVALID_ID);
        else
          assert(inst->result < fn->value_count);
      }
    }
  }
}

static size_t instructions(Ctx* ctx, IrInstructionType kind) {
  size_t count = 0;
  for (size_t f = 0; f < ctx->ir.functions.count; f++)
    for (size_t b = 0; b < ctx->ir.functions.data[f].blocks.count; b++) {
      IrBlock* block = &ctx->ir.functions.data[f].blocks.data[b];
      for (size_t i = 0; i < block->instructions.count; i++)
        count += block->instructions.data[i].type == kind;
    }
  return count;
}

// clang-format off
describe(lowering) {
  it("lowers an empty inferred Unit main") {
    CHECK_LOWER(PROLOG, MAIN);
    Execution state = {.budget = 100};
    assert(execute(ctx, 0, NULL, &state).type == IR_OP_UNIT);
  }
  it("preserves arithmetic without folding and infers function results") {
    CHECK_LOWER(PROLOG, F_HEAD, INT(1), TOK(OPERATOR_PLUS), INT(2),
      TOK(OPERATOR_MULTIPLY), INT(3), EOL, MAIN);
    Execution state = {.budget = 100};
    asserteq(number(execute(ctx, 0, NULL, &state)), 7);
    asserteq(instructions(ctx, IR_INST_MUL), 1);
    asserteq(instructions(ctx, IR_INST_ADD), 1);
  }
  it("registers forward calls and parameter slots before bodies") {
    CHECK_LOWER(PROLOG,
      LINE(0), TOK(DEF), ID("f"), TOK(L_PAREN), ID("n"), TOK(COLON), TOK(TYPE_INT), TOK(R_PAREN), TOK(ASSIGN), ID("g"), TOK(L_PAREN), ID("n"), TOK(R_PAREN), EOL,
      LINE(0), TOK(DEF), ID("g"), TOK(L_PAREN), ID("n"), TOK(COLON), TOK(TYPE_INT), TOK(R_PAREN), TOK(ASSIGN), ID("n"), TOK(OPERATOR_MULTIPLY), INT(2), EOL, MAIN);
    IrOperand arg = {.type = IR_OP_INT32, .value.int32_value = 21};
    Execution state = {.budget = 100};
    asserteq(number(execute(ctx, 0, &arg, &state)), 42);
  }
  it("updates mutable slots and merges a value-producing conditional") {
    CHECK_LOWER(PROLOG, F_HEAD, EOL,
      LINE(2), TOK(VAR), ID("x"), TOK(ASSIGN), INT(0), EOL,
      LINE(2), TOK(WHILE), ID("x"), TOK(COMPARISON_L), INT(3), TOK(DO), EOL,
      LINE(4), ID("x"), TOK(ASSIGN), ID("x"), TOK(OPERATOR_PLUS), INT(1), EOL,
      LINE(2), TOK(IF), ID("x"), TOK(COMPARISON_EQ), INT(3), TOK(THEN), INT(10), TOK(ELSE), INT(20), EOL, MAIN);
    Execution state = {.budget = 100};
    asserteq(number(execute(ctx, 0, NULL, &state)), 10);
  }
  it("keeps shadowed and unreachable declarations in semantic slot order") {
    CHECK_LOWER(PROLOG, LINE(0), TOK(DEF), ID("f"), UNIT_VALUE, TOK(COLON), TOK(TYPE_INT), TOK(ASSIGN), EOL,
      LINE(2), TOK(VAL), ID("x"), TOK(ASSIGN), INT(4), EOL,
      LINE(2), TOK(IF), TOK(BOOL_TRUE), TOK(THEN), EOL,
      LINE(4), TOK(VAL), ID("x"), TOK(ASSIGN), ID("x"), TOK(OPERATOR_PLUS), INT(1), EOL,
      LINE(4), TOK(RETURN), ID("x"), EOL,
      LINE(2), TOK(RETURN), ID("x"), EOL,
      LINE(2), TOK(VAL), ID("unused"), TOK(ASSIGN), INT(99), EOL,
      LINE(2), INT(0), EOL, MAIN);
    Execution state = {.budget = 100};
    asserteq(number(execute(ctx, 0, NULL, &state)), 5);
    asserteq(ctx->ir.functions.data[0].slots.count, 3);
  }
  it("short-circuits AND and OR calls") {
    CHECK_LOWER(PROLOG, F_HEAD, TOK(BOOL_FALSE), TOK(BOOL_AND), ID("readBoolean"), UNIT_VALUE, EOL,
      LINE(0), TOK(DEF), ID("g"), UNIT_VALUE, TOK(ASSIGN), TOK(BOOL_TRUE), TOK(BOOL_OR), ID("readBoolean"), UNIT_VALUE, EOL, MAIN);
    Execution state = {.budget = 100, .input = 1};
    asserteq(number(execute(ctx, 0, NULL, &state)), 0);
    asserteq(number(execute(ctx, 1, NULL, &state)), 1);
    asserteq(state.reads, 0);
  }
  it("evaluates the right side when short-circuiting requires it") {
    CHECK_LOWER(PROLOG, F_HEAD, TOK(BOOL_TRUE), TOK(BOOL_AND), ID("readBoolean"), UNIT_VALUE, EOL, MAIN);
    Execution state = {.budget = 100, .input = 1};
    asserteq(number(execute(ctx, 0, NULL, &state)), 1);
    asserteq(state.reads, 1);
  }
  it("preserves discarded Unit-returning function effects") {
    CHECK_LOWER(PROLOG, LINE(0), TOK(DEF), ID("f"), UNIT_VALUE, TOK(COLON), TOK(TYPE_UNIT), TOK(ASSIGN), ID("readInt"), UNIT_VALUE, EOL, MAIN);
    Execution state = {.budget = 100, .input = 9};
    assert(execute(ctx, 0, NULL, &state).type == IR_OP_UNIT);
    asserteq(state.reads, 1);
  }
  it("uses one early return per closed branch and supports recursion") {
    CHECK_LOWER(PROLOG, LINE(0), TOK(DEF), ID("f"), TOK(L_PAREN), ID("n"), TOK(COLON), TOK(TYPE_INT), TOK(R_PAREN), TOK(COLON), TOK(TYPE_INT), TOK(ASSIGN), EOL,
      LINE(2), TOK(IF), ID("n"), TOK(COMPARISON_LE), INT(0), TOK(THEN), TOK(RETURN), INT(0), TOK(ELSE), TOK(RETURN), ID("f"), TOK(L_PAREN), ID("n"), TOK(OPERATOR_MINUS), INT(1), TOK(R_PAREN), TOK(OPERATOR_PLUS), INT(1), EOL, MAIN);
    IrOperand arg = {.type = IR_OP_INT32, .value.int32_value = 4};
    Execution state = {.budget = 100};
    asserteq(number(execute(ctx, 0, &arg, &state)), 4);
  }
  it("lowers negative range steps and guards") {
    CHECK_LOWER(PROLOG, F_HEAD, EOL,
      LINE(2), TOK(VAR), ID("sum"), TOK(ASSIGN), INT(0), EOL,
      LINE(2), TOK(FOR), ID("i"), TOK(CONTROL_ARROW), INT(3), ID("to"), INT(1), ID("by"), TOK(OPERATOR_MINUS), INT(1), TOK(IF), ID("i"), TOK(COMPARISON_NEQ), INT(2), TOK(DO), EOL,
      LINE(4), ID("sum"), TOK(ASSIGN), ID("sum"), TOK(OPERATOR_PLUS), ID("i"), EOL,
      LINE(2), ID("sum"), EOL, MAIN);
    Execution state = {.budget = 100};
    asserteq(number(execute(ctx, 0, NULL, &state)), 4);
  }
  it("exits 40 on a dynamic zero step") {
    CHECK_LOWER(PROLOG, F_HEAD, EOL,
      LINE(2), TOK(VAL), ID("step"), TOK(ASSIGN), ID("readInt"), UNIT_VALUE, EOL,
      LINE(2), TOK(FOR), ID("i"), TOK(CONTROL_ARROW), INT(1), ID("until"), INT(3), ID("by"), ID("step"), TOK(DO), UNIT_VALUE, EOL,
      LINE(2), INT(9), EOL, MAIN);
    Execution state = {.budget = 100, .input = 0};
    execute(ctx, 0, NULL, &state);
    asserteq(state.exit_code, 40);
    asserteq(state.reads, 1);
  }
  it("consumes widening, division and floor member nodes") {
    CHECK_LOWER(PROLOG, F_HEAD, TOK(L_PAREN), INT(5), TOK(OPERATOR_PLUS), DBL(0.5), TOK(R_PAREN), TOK(PERIOD), ID("floor"), TOK(OPERATOR_DIVIDE), DBL(2.0), EOL, MAIN);
    Execution state = {.budget = 100};
    asserteq(number(execute(ctx, 0, NULL, &state)), 2.5);
    asserteq(instructions(ctx, IR_INST_INT2DOUBLE), 1);
    asserteq(instructions(ctx, IR_INST_FLOOR), 1);
  }
  it("emits concatenation and conversion calls for interpolation and members") {
    CHECK_LOWER(PROLOG, F_HEAD, S_OPEN, STR("value="), DOLLAR_OPEN, INT(7), DOLLAR_CLOSE, S_CLOSE, TOK(PERIOD), ID("length"), EOL, MAIN);
    assert(instructions(ctx, IR_INST_CONCAT) >= 2);
    assert(instructions(ctx, IR_INST_CALL) >= 2);
  }
  it("handles zero iterations and an exclusive range with the default step") {
    CHECK_LOWER(PROLOG,
      LINE(0), TOK(DEF), ID("f"), TOK(L_PAREN), ID("n"), TOK(COLON), TOK(TYPE_INT), TOK(R_PAREN), TOK(ASSIGN), EOL,
      LINE(2), TOK(VAR), ID("sum"), TOK(ASSIGN), INT(0), EOL,
      LINE(2), TOK(FOR), ID("i"), TOK(CONTROL_ARROW), INT(1), ID("until"), ID("n"), TOK(DO), EOL,
      LINE(4), ID("sum"), TOK(ASSIGN), ID("sum"), TOK(OPERATOR_PLUS), ID("i"), EOL,
      LINE(2), ID("sum"), EOL, MAIN);
    for (int bound = 0; bound <= 4; bound++) {
      IrOperand arg = {.type = IR_OP_INT32, .value.int32_value = bound};
      Execution state = {.budget = 100};
      asserteq(number(execute(ctx, 0, &arg, &state)), bound <= 1 ? 0 : bound * (bound - 1) / 2);
    }
  }
  it("evaluates each range bound once before iteration") {
    CHECK_LOWER(PROLOG, F_HEAD, EOL,
      LINE(2), TOK(FOR), ID("i"), TOK(CONTROL_ARROW), ID("readInt"), UNIT_VALUE, ID("to"), ID("readInt"), UNIT_VALUE, TOK(DO), UNIT_VALUE, EOL,
      LINE(2), INT(9), EOL, MAIN);
    Execution state = {.budget = 100, .input = 1};
    asserteq(number(execute(ctx, 0, NULL, &state)), 9);
    asserteq(state.reads, 2);
  }
  it("returns from a loop without executing the following statements") {
    CHECK_LOWER(PROLOG, LINE(0), TOK(DEF), ID("f"), UNIT_VALUE, TOK(COLON), TOK(TYPE_INT), TOK(ASSIGN), EOL,
      LINE(2), TOK(WHILE), TOK(BOOL_TRUE), TOK(DO), TOK(RETURN), INT(7), EOL,
      LINE(2), INT(99), EOL, MAIN);
    Execution state = {.budget = 100};
    asserteq(number(execute(ctx, 0, NULL, &state)), 7);
  }
  it("cleans up partial IR for every injected lowering allocation failure") {
    PARSE(PROLOG, F_HEAD, ID("print"), TOK(L_PAREN), ID("readInt"), UNIT_VALUE, TOK(R_PAREN), EOL, MAIN);
    semantics_analyze(ctx);
    bool reached_success = false;
    for (int fail = 1; fail < 100; fail++) {
      ir_free(&ctx->ir);
      ir_init(&ctx->ir);
      ctx->exit_code = E_OK;
      ctx->flags.forceNthMallocFail = fail;
      if (setjmp(ctx->error_target) == 0) {
        lower_ast(ctx);
        validate_ir(ctx);
        reached_success = true;
        break;
      }
      asserteq(ctx->exit_code, E_INTERNAL_ERROR);
    }
    ctx->flags.forceNthMallocFail = 0;
    assert(reached_success);
  }
}
// clang-format on
snow_main();
