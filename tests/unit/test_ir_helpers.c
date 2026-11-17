/**
 * @file test_ir_helpers.c
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 3. 11. 2026
 *
 * @brief Unit test for the intermediate representation construction helpers
 */

#include "ir.h"
#define SNOW_ENABLED 1
#include "snow/snow.h"

#include "ir_helpers.h"

#include "array.h"

describe(ir_helpers) {
  subdesc(ir_add_function) {
    it("initializes the function") {
      char* name = "main";
      IrFunctionId id = ir_add_function(ctx, name, IR_INT);
      IrFunction* function = &ctx->ir.functions.data[id];

      asserteq(id, 0);
      asserteq_str(function->name, name);
      assertneq_ptr(&function->name, &name, "name should be copied");
      asserteq(function->return_type, IR_INT);
      asserteq(function->entrypoint, IR_INVALID_ID);
      asserteq(function->slots.count, 0);
      asserteq(function->parameter_count, 0);
      asserteq(function->blocks.count, 0);
      asserteq(function->value_count, 0);
    }

    it("appends functions") {
      IrFunctionId first = ir_add_function(ctx, "first", IR_INT);
      IrFunctionId second = ir_add_function(ctx, "second", IR_BOOL);

      asserteq(first, 0);
      asserteq(second, 1);
      asserteq(ctx->ir.functions.count, 2);
      asserteq_str(ctx->ir.functions.data[first].name, "first");
    }

    it("throws on allocation failure") {
      ctx->flags.forceNthMallocFail = 1;

      assertThrow(E_INTERNAL_ERROR, { ir_add_function(ctx, "main", IR_UNIT); });
      asserteq(ctx->ir.functions.count, 0);
    }
  }

  subdesc(ir_add_slot) {
    before_each() {
      ir_add_function(ctx, "main", IR_UNIT);
    }

    it("adds locals after parameters") {
      ir_add_parameter(ctx, 0, "parameter", IR_BOOL);
      IrStackSlotId id = ir_add_slot(ctx, 0, "local", IR_STRING);

      asserteq(id, 1);
      asserteq(ctx->ir.functions.data[0].parameter_count, 1);
      asserteq(ctx->ir.functions.data[0].slots.count, 2);
      asserteq(ctx->ir.functions.data[0].slots.data[id].type, IR_STRING);
      asserteq(ctx->ir.functions.data[0].slots.data[id].lastStore, NULL);
    }

    it("copies the name") {
      char name[] = "local";
      ir_add_slot(ctx, 0, name, IR_INT);

      asserteq_str(ctx->ir.functions.data[0].slots.data[0].name, name);
      assertneq_ptr(&ctx->ir.functions.data[0].slots.data[0].name, &name,
                    "name should be copied");
    }

    it("throws if copying the name fails") {
      ctx->flags.forceNthMallocFail = 2;

      assertThrow(E_INTERNAL_ERROR, { ir_add_slot(ctx, 0, "local", IR_INT); });
      asserteq(ctx->ir.functions.data[0].slots.count, 0);
    }
  }

  subdesc(ir_add_parameter) {
    before_each() {
      ir_add_function(ctx, "main", IR_UNIT);
    }

    it("adds a parameter slot") {
      char* name = "value";
      IrStackSlotId id = ir_add_parameter(ctx, 0, name, IR_INT);
      IrStackSlot* slot = &ctx->ir.functions.data[0].slots.data[id];

      asserteq(id, 0);
      asserteq(ctx->ir.functions.data[0].parameter_count, 1);
      asserteq(ctx->ir.functions.data[0].slots.count, 1);
      asserteq_str(slot->name, name);
      assertneq_ptr(&slot->name, &name, "name should be copied");
      asserteq(slot->type, IR_INT);
      asserteq(slot->lastStore, NULL);
    }

    it("rejects parameters after local variables") {
      ir_add_slot(ctx, 0, "local", IR_INT);

      assertThrow(E_INTERNAL_ERROR,
                  { ir_add_parameter(ctx, 0, "parameter", IR_DOUBLE); });
      asserteq(ctx->ir.functions.data[0].parameter_count, 0);
      asserteq(ctx->ir.functions.data[0].slots.count, 1);
      asserteq_str(ctx->ir.functions.data[0].slots.data[0].name, "local");
      asserteq(ctx->ir.functions.data[0].slots.data[0].type, IR_INT);
      asserteq(ctx->ir.functions.data[0].slots.data[0].lastStore, NULL);
    }
  }

  subdesc(ir_add_block) {
    before_each() {
      ir_add_function(ctx, "main", IR_UNIT);
    }

    it("initializes an empty block") {
      IrBlockId id = ir_add_block(ctx, 0);
      IrBlock* block = &ctx->ir.functions.data[0].blocks.data[id];

      asserteq(id, 0);
      assert(block->instructions.data == NULL);
      asserteq(block->instructions.count, 0);
      asserteq(block->instructions.capacity, 0);
      asserteq(block->terminator.type, IR_TERMINATOR_JUMP);
      asserteq(block->terminator.value.jump_target, IR_INVALID_ID);
    }

    it("appends blocks") {
      IrBlockId first = ir_add_block(ctx, 0);
      IrBlockId second = ir_add_block(ctx, 0);

      asserteq(first, 0);
      asserteq(second, 1);
      asserteq(ctx->ir.functions.data[0].blocks.count, 2);
    }

    it("rejects an invalid function id") {
      assertThrow(E_INTERNAL_ERROR, { ir_add_block(ctx, IR_INVALID_ID); });
    }
  }

  subdesc(ir_add_instruction) {
    before_each() {
      ir_add_function(ctx, "main", IR_INT);
      ir_add_block(ctx, 0);
    }

    it("numbers results across blocks") {
      ir_add_block(ctx, 0);
      IrInstruction instruction = {.type = IR_INST_LOAD, .result_type = IR_INT};

      IrValueId first = ir_add_instruction(ctx, 0, 0, instruction);
      IrValueId second = ir_add_instruction(ctx, 0, 1, instruction);

      asserteq(first, 0);
      asserteq(second, 1);
      asserteq(ctx->ir.functions.data[0].value_count, 2);
    }

    it("does not assign a result to stores") {
      IrInstruction instruction = {.type = IR_INST_STORE};

      IrValueId result = ir_add_instruction(ctx, 0, 0, instruction);

      asserteq(result, IR_INVALID_ID);
      asserteq(ctx->ir.functions.data[0].value_count, 0);
    }

    it("does not assign a result to unit calls") {
      IrInstruction instruction = {.type = IR_INST_CALL,
                                   .operands.call.type = IR_FX_PRINT,
                                   .result_type = IR_UNIT};

      IrValueId result = ir_add_instruction(ctx, 0, 0, instruction);

      asserteq(result, IR_INVALID_ID);
      asserteq(ctx->ir.functions.data[0].value_count, 0);
    }

    it("copies string operands") {
      char* text = "text";
      IrOperand operand = {.type = IR_OP_STRING, .value.string_value = text};
      IrInstruction instruction = {.type = IR_INST_CONCAT,
                                   .operands.binary = {operand, operand},
                                   .result_type = IR_STRING};
      ir_add_instruction(ctx, 0, 0, instruction);

      IrInstruction* copy =
          &ctx->ir.functions.data[0].blocks.data[0].instructions.data[0];
      asserteq_str(copy->operands.binary.left.value.string_value, text);
      assertneq_ptr(&copy->operands.binary.left.value.string_value, &text,
                    "string should be copied");
      asserteq_str(copy->operands.binary.right.value.string_value, text);
      assertneq_ptr(&copy->operands.binary.right.value.string_value, &text,
                    "string should be copied");
    }

    it("copies unary operands") {
      IrInstruction instruction = {
          .type = IR_INST_NOT,
          .operands.unary.operand = {.type = IR_OP_BOOL,
                                     .value.bool_value = true},
          .result_type = IR_BOOL};

      ir_add_instruction(ctx, 0, 0, instruction);

      IrInstruction* copy =
          &ctx->ir.functions.data[0].blocks.data[0].instructions.data[0];
      asserteq(copy->operands.unary.operand.type, IR_OP_BOOL);
      assert(copy->operands.unary.operand.value.bool_value);
    }

    it("copies call arguments") {
      char text[] = "text";
      IrOperand args[] = {{.type = IR_OP_STRING, .value.string_value = text}};
      IrInstruction instruction = {
          .type = IR_INST_CALL,
          .operands.call = {.type = IR_FX_PRINT, .args = args, .arg_count = 1},
          .result_type = IR_UNIT};
      ir_add_instruction(ctx, 0, 0, instruction);
      args[0].type = IR_OP_UNIT;
      text[0] = 'X';

      IrInstruction* copy =
          &ctx->ir.functions.data[0].blocks.data[0].instructions.data[0];
      asserteq(copy->operands.call.arg_count, 1);
      asserteq(copy->operands.call.args[0].type, IR_OP_STRING);
      asserteq_str(copy->operands.call.args[0].value.string_value, "text");
    }

    it("can free a partially copied instruction") {
      IrOperand text = {.type = IR_OP_STRING, .value.string_value = "text"};
      IrInstruction instruction = {.type = IR_INST_CONCAT,
                                   .operands.binary = {text, text},
                                   .result_type = IR_STRING};
      // Fail after the first string has already been copied
      ctx->flags.forceNthMallocFail = 3;

      assertThrow(E_INTERNAL_ERROR,
                  { ir_add_instruction(ctx, 0, 0, instruction); });
      asserteq(ctx_free(ctx), E_INTERNAL_ERROR);
      assert(ctx->ir.functions.data == NULL);
    }

    it("rejects an invalid block id") {
      IrInstruction instruction = {.type = IR_INST_LOAD, .result_type = IR_INT};

      assertThrow(E_INTERNAL_ERROR,
                  { ir_add_instruction(ctx, 0, IR_INVALID_ID, instruction); });
    }
  }

  subdesc(ir_set_terminator) {
    before_each() {
      ir_add_function(ctx, "main", IR_STRING);
      ir_add_block(ctx, 0);
    }

    it("sets a jump target") {
      IrBlockTerminator terminator = {.type = IR_TERMINATOR_JUMP,
                                      .value.jump_target = 0};

      ir_set_terminator(ctx, 0, 0, terminator);

      asserteq(
          ctx->ir.functions.data[0].blocks.data[0].terminator.value.jump_target,
          0);
    }

    it("copies return strings") {
      char* text = "text";
      IrBlockTerminator terminator = {
          .type = IR_TERMINATOR_RETURN,
          .value.return_value = {.type = IR_OP_STRING,
                                 .value.string_value = text}};
      ir_set_terminator(ctx, 0, 0, terminator);

      IrBlockTerminator* copy =
          &ctx->ir.functions.data[0].blocks.data[0].terminator;
      asserteq(copy->type, IR_TERMINATOR_RETURN);
      asserteq_str(copy->value.return_value.value.string_value, text);
      assertneq_ptr(&copy->value.return_value.value.string_value, &text,
                    "string should be copied");
    }

    it("preserves the old terminator if copying fails") {
      IrBlockTerminator terminator = {
          .type = IR_TERMINATOR_RETURN,
          .value.return_value = {.type = IR_OP_STRING,
                                 .value.string_value = "text"}};
      ctx->flags.forceNthMallocFail = 1;

      assertThrow(E_INTERNAL_ERROR,
                  { ir_set_terminator(ctx, 0, 0, terminator); });
      asserteq(ctx->ir.functions.data[0].blocks.data[0].terminator.type,
               IR_TERMINATOR_JUMP);
    }
  }
}

snow_main();
