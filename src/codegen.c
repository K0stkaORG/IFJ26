/**
 * @file codegen.c
 * Project: IFJ26
 *
 * @author David Daniel Slapal (xslapad00)
 * @date 14. 11. 2026
 *
 * @brief IFJcode26 generator from Intermediate Representation
 */

#include "codegen.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "ctx.h"
#include "ir.h"

// Code structure

/**
 * @brief Print new line
 * @param out File to print to
 */
static void cg_print_nl(FILE* out) {
  fputs("\n", out);
}

/**
 * @brief Print a space
 * @param out File to print to
 */
static void cg_print_ws(FILE* out) {
  fputs(" ", out);
}

/**
 * @brief Print IFJcode26 preamble
 * @param out File to print to
 */
static void cg_print_preamble(FILE* out) {
  fputs(".IFJcode26", out);
  cg_print_nl(out);
}

// Data types

/**
 * @brief Print int in IFJcode26
 * @param out File to print to
 * @param value value to print
 */
static void cg_print_int(FILE* out, IrInt value) {
  fprintf(out, "int@%ld", value);
}

/**
 * @brief Print float in IFJcode26
 * @param out File to print to
 * @param value value to print
 */
static void cg_print_float(FILE* out, IrDouble value) {
  fprintf(out, "float@%a", value);
}

/**
 * @brief Print bool in IFJcode26
 * @param out File to print to
 * @param value value to print
 */
static void cg_print_bool(FILE* out, IrBool value) {
  fputs(value ? "bool@true" : "bool@false", out);
}

/**
 * @brief Print nil in IFJcode26
 * @param out File to print to
 */
static void cg_print_nil(FILE* out) {
  fputs("nil@nil", out);
}

/**
 * @brief Escape and print string in IFJcode26
 * @param out File to print to
 * @param str value to print
 */
static void cg_print_string(FILE* out, char* str) {
  fputs("string@", out);
  unsigned char* s = (unsigned char*)str;
  size_t s_len = strlen(str);
  for (size_t i = 0; i < s_len; i++) {
    if (s[i] <= 32 || s[i] == 35 || s[i] == 92 || s[i] >= 127) {
      fprintf(out, "\\%03u", s[i]);
    } else {
      fputc(s[i], out);
    }
  }
}

// Operands

/**
 * @brief Print LF@val
 * @param out File to print to
 * @param id ID of value
 */
static void cg_print_localframe_val(FILE* out, size_t id) {
  fprintf(out, "LF@val_%zu", id);
}

/**
 * @brief Print LF@slot
 * @param out File to print to
 * @param id ID of slot
 */
static void cg_print_localframe_slot(FILE* out, size_t id) {
  fprintf(out, "LF@slot_%zu", id);
}

/**
 * @brief Print TF@slot
 * @param out File to print to
 * @param id ID of slot
 */
static void cg_print_tempframe_slot(FILE* out, size_t id) {
  fprintf(out, "TF@slot_%zu", id);
}

/**
 * @brief Print operand
 * @param out File to print to
 * @param op operand
 */
static void cg_print_operand(FILE* out, IrOperand* op) {
  switch (op->type) {
    case IR_OP_VALUE:
      cg_print_localframe_val(out, op->value.id);
      break;
    case IR_OP_INT32:
      cg_print_int(out, op->value.int32_value);
      break;
    case IR_OP_DOUBLE:
      cg_print_float(out, op->value.double_value);
      break;
    case IR_OP_BOOL:
      cg_print_bool(out, op->value.bool_value);
      break;
    case IR_OP_STRING:
      cg_print_string(out, op->value.string_value);
      break;
    case IR_OP_UNIT:
      cg_print_nil(out);
      break;
  }
}

// Instructions

/**
 * @brief Print unary instruction
 * @param out File to print to
 * @param opcode opcode
 * @param inst instruction
 */
static void cg_print_unary_inst(FILE* out, char* opcode, IrInstruction* inst) {
  if (inst->result == IR_INVALID_ID)
    return;
  fputs(opcode, out);
  cg_print_ws(out);
  cg_print_localframe_val(out, inst->result);
  cg_print_ws(out);
  cg_print_operand(out, &inst->operands.unary.operand);
  cg_print_nl(out);
}

/**
 * @brief Print binary instruction
 * @param out File to print to
 * @param opcode opcode
 * @param inst instruction
 */
static void cg_print_binary_inst(FILE* out, char* opcode, IrInstruction* inst) {
  if (inst->result == IR_INVALID_ID && strcmp(opcode, "DIV") != 0 &&
      strcmp(opcode, "IDIV") != 0) {
    return;
  }
  fputs(opcode, out);
  cg_print_ws(out);
  if (inst->result == IR_INVALID_ID) {
    fputs("GF@$tmp1", out);
  } else {
    cg_print_localframe_val(out, inst->result);
  }
  cg_print_ws(out);
  cg_print_operand(out, &inst->operands.binary.left);
  cg_print_ws(out);
  cg_print_operand(out, &inst->operands.binary.right);
  cg_print_nl(out);
}

/**
 * @brief Print negated value of result
 * @param out File to print to
 * @param result result
 */
static void cg_print_not_result(FILE* out, size_t result) {
  if (result == IR_INVALID_ID)
    return;
  fputs("NOT ", out);
  cg_print_localframe_val(out, result);
  cg_print_ws(out);
  cg_print_localframe_val(out, result);
  cg_print_nl(out);
}

/**
 * @brief Print inline builtin function execution
 * @param out File to print to
 * @param inst instruction
 */
static void cg_print_builtin(Ctx* ctx, FILE* out, IrInstruction* inst) {
  switch (inst->operands.call.type) {
    // verify helper enough of GF vars is created for every inline
    case IR_FX_STR_LEN:
      if (inst->result == IR_INVALID_ID)
        break;
      fputs("STRLEN ", out);
      cg_print_localframe_val(out, inst->result);
      cg_print_ws(out);
      cg_print_operand(out, &inst->operands.call.args[0]);
      cg_print_nl(out);
      break;
    case IR_FX_TO_INT:
      if (inst->result == IR_INVALID_ID)
        break;
      fputs("FLOAT2INT ", out);
      cg_print_localframe_val(out, inst->result);
      cg_print_ws(out);
      cg_print_operand(out, &inst->operands.call.args[0]);
      cg_print_nl(out);
      break;
    case IR_FX_CHR:
      fputs("INT2CHAR ", out);
      if (inst->result == IR_INVALID_ID) {
        fputs("GF@$tmp1", out);
      } else {
        cg_print_localframe_val(out, inst->result);
      }
      cg_print_ws(out);
      cg_print_operand(out, &inst->operands.call.args[0]);
      cg_print_nl(out);
      break;
    case IR_FX_PRINT:
      break;
    case IR_FX_READ_INT:
      break;
    case IR_FX_READ_DOUBLE:
      break;
    case IR_FX_READ_BOOL:
      break;
    case IR_FX_READ_LINE:
      break;
    case IR_FX_TO_STR:
      break;
    case IR_FX_STR_SUB:
      break;
    case IR_FX_ORD:
      break;

    case IR_FX_USER:  // silence warning
      break;
  }
}

/**
 * @brief Generate inline instruction
 * @param ctx Context
 * @param out File to print to
 * @param inst instruction
 */
static void cg_generate_instruction(Ctx* ctx, FILE* out, IrInstruction* inst) {
  switch (inst->type) {
    case IR_INST_ADD:
      cg_print_binary_inst(out, "ADD", inst);
      break;
    case IR_INST_SUB:
      cg_print_binary_inst(out, "SUB", inst);
      break;
    case IR_INST_MUL:
      cg_print_binary_inst(out, "MUL", inst);
      break;
    case IR_INST_DIV:
      cg_print_binary_inst(out, "DIV", inst);
      break;
    case IR_INST_IDIV:
      cg_print_binary_inst(out, "IDIV", inst);
      break;
    case IR_INST_FLOOR:
      cg_print_unary_inst(out, "FLOAT2INT", inst);
      break;
    case IR_INST_CONCAT:
      cg_print_binary_inst(out, "CONCAT", inst);
      break;
    case IR_INST_NOT:
      cg_print_unary_inst(out, "NOT", inst);
      break;
    case IR_INST_INT2DOUBLE:
      cg_print_unary_inst(out, "INT2FLOAT", inst);
      break;
    case IR_INST_EQ:
      cg_print_binary_inst(out, "EQ", inst);
      break;
    case IR_INST_LT:
      cg_print_binary_inst(out, "LT", inst);
      break;
    case IR_INST_GT:
      cg_print_binary_inst(out, "GT", inst);
      break;
    case IR_INST_NE:
      cg_print_binary_inst(out, "EQ", inst);
      cg_print_not_result(out, inst->result);
      break;
    case IR_INST_LE:
      cg_print_binary_inst(out, "GT", inst);
      cg_print_not_result(out, inst->result);
      break;
    case IR_INST_GE:
      cg_print_binary_inst(out, "LT", inst);
      cg_print_not_result(out, inst->result);
      break;
    case IR_INST_LOAD:
      if (inst->result == IR_INVALID_ID)
        break;
      fputs("MOVE ", out);
      cg_print_localframe_val(out, inst->result);
      cg_print_ws(out);
      cg_print_localframe_slot(out, inst->operands.load.source);
      cg_print_nl(out);
      break;
    case IR_INST_STORE:
      fputs("MOVE ", out);
      cg_print_localframe_slot(out, inst->operands.store.target);
      cg_print_ws(out);
      cg_print_operand(out, &inst->operands.store.source);
      cg_print_nl(out);
      break;
    case IR_INST_CALL:
      if (inst->operands.call.type == IR_FX_USER) {
        fputs("CREATEFRAME", out);
        cg_print_nl(out);

        for (size_t i = 0; i < inst->operands.call.arg_count; i++) {
          fputs("DEFVAR ", out);
          cg_print_tempframe_slot(out, i);
          cg_print_nl(out);
          fputs("MOVE ", out);
          cg_print_tempframe_slot(out, i);
          cg_print_ws(out);
          cg_print_operand(out, &inst->operands.call.args[i]);
          cg_print_nl(out);
        }

        fputs("CALL $$", out);
        fputs(ctx->ir.functions.data[inst->operands.call.id].name, out);
        cg_print_nl(out);

        if (inst->result != IR_INVALID_ID) {
          IrFunction* callee = &ctx->ir.functions.data[inst->operands.call.id];
          fputs("MOVE ", out);
          cg_print_localframe_val(out, inst->result);
          if (callee->return_type == IR_UNIT) {
            fputs(" nil@nil", out);
          } else {
            fputs(" TF@%ret", out);
          }
          cg_print_nl(out);
        }
      } else {
        cg_print_builtin(ctx, out, inst);
      }
      break;
  }
}

// Blocks

/**
 * @brief Print block label
 * @param out File to print to
 * @param fn_name function name
 * @param block_id ID of the block
 */
static void cg_print_block_label(FILE* out, char* fn_name, size_t block_id) {
  fprintf(out, "$%s$b%zu", fn_name, block_id);
}

/**
 * @brief Generate block
 * @param ctx Context
 * @param out File to print to
 * @param fn function
 * @param block_id ID of the block
 * @param block block
 */
static void cg_generate_block(Ctx* ctx,
                              FILE* out,
                              IrFunction* fn,
                              size_t block_id,
                              IrBlock* block) {
  fputs("LABEL ", out);
  cg_print_block_label(out, fn->name, block_id);
  cg_print_nl(out);

  for (size_t i = 0; i < block->instructions.count; i++) {
    IrInstruction* inst = &block->instructions.data[i];
    cg_generate_instruction(ctx, out, inst);
  }

  switch (block->terminator.type) {
    case IR_TERMINATOR_EXIT:
      fputs("EXIT ", out);
      cg_print_operand(out, &block->terminator.value.exit_code);
      cg_print_nl(out);
      break;
    case IR_TERMINATOR_JUMP:
      if (block->terminator.value.jump_target != IR_INVALID_ID) {
        fputs("JUMP ", out);
        cg_print_block_label(out, fn->name,
                             block->terminator.value.jump_target);
        cg_print_nl(out);
      }
      break;
    case IR_TERMINATOR_RETURN:
      if (fn->return_type != IR_UNIT) {
        fputs("MOVE LF@%ret ", out);
        cg_print_operand(out, &block->terminator.value.return_value);
        cg_print_nl(out);
      }
      fputs("POPFRAME", out);
      cg_print_nl(out);
      fputs("RETURN", out);
      cg_print_nl(out);
      break;
    case IR_TERMINATOR_CONDITIONAL:
      fputs("JUMPIFEQ ", out);
      cg_print_block_label(
          out, fn->name,
          block->terminator.value.conditional_target.true_target);
      cg_print_ws(out);
      cg_print_operand(out,
                       &block->terminator.value.conditional_target.condition);
      cg_print_ws(out);
      cg_print_bool(out, true);
      cg_print_nl(out);
      fputs("JUMP ", out);
      cg_print_block_label(
          out, fn->name,
          block->terminator.value.conditional_target.false_target);
      cg_print_nl(out);
      break;
  }
}

// Functions

/**
 * @brief Generate function
 * @param ctx Context
 * @param out File to print to
 * @param fn function
 */
static void cg_generate_function(Ctx* ctx, FILE* out, IrFunction* fn) {
  fputs("LABEL $$", out);
  fputs(fn->name, out);
  cg_print_nl(out);
  fputs("PUSHFRAME", out);
  cg_print_nl(out);

  if (fn->return_type != IR_UNIT) {
    fputs("DEFVAR LF@%ret", out);
    cg_print_nl(out);
  }

  for (size_t i = 0; i < fn->value_count; i++) {
    fputs("DEFVAR ", out);
    cg_print_localframe_val(out, i);
    cg_print_nl(out);
  }

  for (size_t i = fn->parameter_count; i < fn->slots.count; i++) {
    fputs("DEFVAR ", out);
    cg_print_localframe_slot(out, i);
    cg_print_nl(out);
  }

  if (fn->entrypoint != IR_INVALID_ID && fn->entrypoint != 0) {
    fputs("JUMP ", out);
    cg_print_block_label(out, fn->name, fn->entrypoint);
    cg_print_nl(out);
  }

  for (size_t i = 0; i < fn->blocks.count; i++) {
    IrBlock* block = &fn->blocks.data[i];
    cg_generate_block(ctx, out, fn, i, block);
  }
}

// Generator

void cg_generate(FILE* out, Ctx* ctx) {
  cg_state_init(&ctx->cg_state);
  cg_print_preamble(out);

  // AI generated by: Antigravity / Gemini 3.8 Flash
  // Prompt: "Modify cg_generate right after preamble defining a helper size_t
  // and iterating through all calls. Note the maximal amount of helper
  // variables DEFVAR GF@$tmpX that need to be defined. If IR_FX_STR_SUB is
  // present: 3, IR_FX_ORD: 1, IR_FX_READ_*: 1, IR_FX_TO_STR: 1." Changes:
  // Iterate through all IR calls to determine the maximum number of needed
  // helper variables and emit DEFVAR GF@$tmpX right after preamble.
  size_t max_helper_vars = 0;
  for (size_t i = 0; i < ctx->ir.functions.count; i++) {
    IrFunction* fn = &ctx->ir.functions.data[i];
    for (size_t j = 0; j < fn->blocks.count; j++) {
      IrBlock* block = &fn->blocks.data[j];
      for (size_t k = 0; k < block->instructions.count; k++) {
        IrInstruction* inst = &block->instructions.data[k];
        size_t needed = 0;
        if (inst->type == IR_INST_CALL &&
            inst->operands.call.type != IR_FX_USER) {
          IrFunctionType fx_type = inst->operands.call.type;
          if (fx_type == IR_FX_STR_SUB) {
            needed = 3;
          } else if (fx_type == IR_FX_ORD || fx_type == IR_FX_TO_STR ||
                     fx_type == IR_FX_READ_LINE || fx_type == IR_FX_READ_INT ||
                     fx_type == IR_FX_READ_BOOL ||
                     fx_type == IR_FX_READ_DOUBLE ||
                     (fx_type == IR_FX_CHR && inst->result == IR_INVALID_ID)) {
            needed = 1;
          }
        } else if ((inst->type == IR_INST_DIV || inst->type == IR_INST_IDIV) &&
                   inst->result == IR_INVALID_ID) {
          needed = 1;
        }
        if (needed > max_helper_vars) {
          max_helper_vars = needed;
        }
      }
    }
  }

  for (size_t i = 1; i <= max_helper_vars; i++) {
    fprintf(out, "DEFVAR GF@$tmp%zu", i);
    cg_print_nl(out);
  }

  if (ctx->ir.entrypoint != IR_INVALID_ID) {
    fputs("CREATEFRAME", out);
    cg_print_nl(out);
    fputs("CALL $$", out);
    fputs(ctx->ir.functions.data[ctx->ir.entrypoint].name, out);
    cg_print_nl(out);
    fputs("EXIT ", out);
    cg_print_int(out, 0);
    cg_print_nl(out);
  }

  for (size_t i = 0; i < ctx->ir.functions.count; i++) {
    IrFunction* fn = &ctx->ir.functions.data[i];
    cg_generate_function(ctx, out, fn);
  }
}

void cg_state_init(CodegenState* state) {
  state->builtin_label_id = 0;
}
