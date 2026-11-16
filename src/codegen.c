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

static void cg_print_nl(FILE* out) {
  fputs("\n", out);
}

static void cg_print_ws(FILE* out) {
  fputs(" ", out);
}

static void cg_print_preamble(FILE* out) {
  fputs(".IFJcode26\n", out);
}

// Data types

static void cg_print_int(FILE* out, IrInt value) {
  fprintf(out, "int@%d", value);
}

static void cg_print_float(FILE* out, IrDouble value) {
  fprintf(out, "float@%a", value);
}

static void cg_print_bool(FILE* out, IrBool value) {
  fputs(value ? "bool@true" : "bool@false", out);
}

static void cg_print_nil(FILE* out) {
  fputs("nil@nil", out);
}

static void cg_print_string(FILE* out, char* str) {
  fputs("string@", out);
  for (size_t i = 0; i < strlen(str); i++) {
    if (str[i] <= 32 || str[i] == 35 || str[i] == 92) {
      fprintf(out, "\\%03u", (unsigned char)str[i]);
    } else {
      fputc(str[i], out);
    }
  }
}

// Operands

static void cg_print_localframe_val(FILE* out, size_t id) {
  fprintf(out, "LF@val_%zu", id);
}

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

static void cg_print_unary_inst(FILE* out, char* opcode, IrInstruction* inst) {
  fputs(opcode, out);
  cg_print_ws(out);
  cg_print_localframe_val(out, inst->result);
  cg_print_ws(out);
  cg_print_operand(out, &inst->operands.unary.operand);
  cg_print_nl(out);
}

static void cg_print_binary_inst(FILE* out, char* opcode, IrInstruction* inst) {
  fputs(opcode, out);
  cg_print_ws(out);
  cg_print_localframe_val(out, inst->result);
  cg_print_ws(out);
  cg_print_operand(out, &inst->operands.binary.left);
  cg_print_ws(out);
  cg_print_operand(out, &inst->operands.binary.right);
  cg_print_nl(out);
}

static void cg_print_not_result(FILE* out, size_t result) {
  fputs("NOT ", out);
  cg_print_localframe_val(out, result);
  cg_print_ws(out);
  cg_print_localframe_val(out, result);
  cg_print_nl(out);
}

static void cg_generate_instruction(FILE* out, IrInstruction* inst) {
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
  }
}

// Generator

void cg_generate(Ctx* ctx, FILE* output) {
  (void)ctx;
  cg_print_preamble(output);
}
