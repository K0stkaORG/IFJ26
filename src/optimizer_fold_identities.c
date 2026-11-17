/**
 * @file optimizer_fold_identities.c
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 15. 11. 2026
 *
 * @brief Helpers for folding identities in the IR
 *
 * This file contains definitions for helper functions for folding identities in
 * the IR.
 */

#include "optimizer_fold_identities.h"

#include "ir.h"

#include <math.h>     // signbit
#include <stdbool.h>  // bool
#include <string.h>   // strlen

bool optimizer_fold_identity(IrInstruction* instruction, IrOperand* result) {
  IrOperand* left = &instruction->operands.binary.left;
  IrOperand* right = &instruction->operands.binary.right;

  switch (instruction->type) {
    case IR_INST_ADD:
      // x + 0 = x
      if (left->type == IR_OP_INT32 && right->type == IR_OP_VALUE &&
          left->value.int32_value == 0) {
        *result = *right;
        return true;
      }

      // 0 + x = x
      if (left->type == IR_OP_VALUE && right->type == IR_OP_INT32 &&
          right->value.int32_value == 0) {
        *result = *left;
        return true;
      }

      return false;

    case IR_INST_SUB:
      // x - 0 = x
      if (left->type == IR_OP_VALUE && right->type == IR_OP_INT32 &&
          right->value.int32_value == 0) {
        *result = *left;
        return true;
      }

      // x - +0.0 = x
      if (left->type == IR_OP_VALUE && right->type == IR_OP_DOUBLE &&
          right->value.double_value == 0.0 &&
          !signbit(right->value.double_value)) {
        *result = *left;
        return true;
      }

      // x - x = 0 (for ints)
      if (left->type == IR_OP_VALUE && right->type == IR_OP_VALUE &&
          left->value.id == right->value.id &&
          instruction->result_type == IR_INT) {
        *result = (IrOperand){IR_OP_INT32, {.int32_value = 0}};
        return true;
      }

      return false;

    case IR_INST_MUL:
      // x * 1 = x
      if (left->type == IR_OP_VALUE && right->type == IR_OP_INT32 &&
          right->value.int32_value == 1) {
        *result = *left;
        return true;
      }

      // 1 * x = x
      if (left->type == IR_OP_INT32 && right->type == IR_OP_VALUE &&
          left->value.int32_value == 1) {
        *result = *right;
        return true;
      }

      // x * 0 = 0
      if (left->type == IR_OP_VALUE && right->type == IR_OP_INT32 &&
          right->value.int32_value == 0) {
        *result = (IrOperand){IR_OP_INT32, {.int32_value = 0}};
        return true;
      }

      // 0 * x = 0
      if (left->type == IR_OP_INT32 && right->type == IR_OP_VALUE &&
          left->value.int32_value == 0) {
        *result = (IrOperand){IR_OP_INT32, {.int32_value = 0}};
        return true;
      }

      // x * 1.0 = x
      if (left->type == IR_OP_VALUE && right->type == IR_OP_DOUBLE &&
          right->value.double_value == 1.0) {
        *result = *left;
        return true;
      }

      // 1.0 * x = x
      if (left->type == IR_OP_DOUBLE && right->type == IR_OP_VALUE &&
          left->value.double_value == 1.0) {
        *result = *right;
        return true;
      }

      return false;

    case IR_INST_IDIV:
      // x / 1 = x
      if (left->type == IR_OP_VALUE && right->type == IR_OP_INT32 &&
          right->value.int32_value == 1) {
        *result = *left;
        return true;
      }

      return false;

    case IR_INST_DIV:
      // x / 1.0 = x
      if (left->type == IR_OP_VALUE && right->type == IR_OP_DOUBLE &&
          right->value.double_value == 1.0) {
        *result = *left;
        return true;
      }

      return false;

    case IR_INST_CONCAT:
      // x + "" = x
      if (left->type == IR_OP_VALUE && right->type == IR_OP_STRING &&
          strlen(right->value.string_value) == 0) {
        *result = *left;
        return true;
      }

      // "" + x = x
      if (left->type == IR_OP_STRING && right->type == IR_OP_VALUE &&
          strlen(left->value.string_value) == 0) {
        *result = *right;
        return true;
      }

      return false;

    // No heuristic matched
    default:
      return false;
  }
}
