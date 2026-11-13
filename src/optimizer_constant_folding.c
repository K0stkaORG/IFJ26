/**
 * @file optimizer_constant_folding.c
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 12. 11. 2026
 *
 * @brief Helpers for folding constants in the IR
 *
 * This file contains definitions for helper functions for folding constants in
 * the IR.
 *
 * @note Folding is done with 64 bit integers as that is what the interpreter
 * uses internally even though IFJ26 and IFJ26Code's integers are supposed to be
 * 32 bit
 */

#include "optimizer_constant_folding.h"

#include "ctx.h"
#include "ir.h"
#include "ir_free_helpers.h"
#include "ir_helpers.h"
#include "optimizer_constant_propagation.h"

#include <math.h>     // floor
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <string.h>   // strcmp

/**
 * @brief Tries to fold a constant in the IR
 *
 * @param[in] instruction The instruction to fold
 * @param[out] result The result of the fold
 * @return Whether the fold was successful
 */
static bool optimizer_fold_instruction(Ctx* ctx,
                                       IrInstruction* instruction,
                                       IrOperand* result) {
  // Handle each instruction class separately
  switch (ir_get_instruction_class(instruction)) {
    case IR_INST_CLASS_UNARY: {
      IrOperand* operand = &instruction->operands.unary.operand;

      // Handle each unary instruction
      switch (instruction->type) {
        case IR_INST_NOT:
          if (operand->type != IR_OP_BOOL)
            return false;

          *result = (IrOperand){IR_OP_BOOL,
                                {.bool_value = !operand->value.bool_value}};
          return true;

        case IR_INST_INT2DOUBLE:
          if (operand->type != IR_OP_INT32)
            return false;

          *result = (IrOperand){
              IR_OP_DOUBLE,
              {.double_value = (IrDouble)operand->value.int32_value}};
          return true;

        case IR_INST_FLOOR:
          if (operand->type != IR_OP_DOUBLE)
            return false;

          *result = (IrOperand){
              IR_OP_DOUBLE,
              {.double_value = (IrDouble)floor(operand->value.double_value)}};
          return true;

        default:
          return false;
      }
    }

    case IR_INST_CLASS_BINARY: {
      IrOperand* left = &instruction->operands.binary.left;
      IrOperand* right = &instruction->operands.binary.right;

      // Handle each binary instruction
      switch (instruction->type) {
        case IR_INST_ADD:
          if (left->type == IR_OP_INT32 && right->type == IR_OP_INT32) {
            *result = (IrOperand){IR_OP_INT32,
                                  {.int32_value = left->value.int32_value +
                                                  right->value.int32_value}};
            return true;
          }

          if (left->type == IR_OP_DOUBLE && right->type == IR_OP_DOUBLE) {
            *result = (IrOperand){IR_OP_DOUBLE,
                                  {.double_value = left->value.double_value +
                                                   right->value.double_value}};
            return true;
          }

          return false;

        case IR_INST_SUB:
          if (left->type == IR_OP_INT32 && right->type == IR_OP_INT32) {
            *result = (IrOperand){IR_OP_INT32,
                                  {.int32_value = left->value.int32_value -
                                                  right->value.int32_value}};
            return true;
          }

          if (left->type == IR_OP_DOUBLE && right->type == IR_OP_DOUBLE) {
            *result = (IrOperand){IR_OP_DOUBLE,
                                  {.double_value = left->value.double_value -
                                                   right->value.double_value}};
            return true;
          }

          return false;

        case IR_INST_MUL:
          if (left->type == IR_OP_INT32 && right->type == IR_OP_INT32) {
            *result = (IrOperand){IR_OP_INT32,
                                  {.int32_value = left->value.int32_value *
                                                  right->value.int32_value}};
            return true;
          }

          if (left->type == IR_OP_DOUBLE && right->type == IR_OP_DOUBLE) {
            *result = (IrOperand){IR_OP_DOUBLE,
                                  {.double_value = left->value.double_value *
                                                   right->value.double_value}};
            return true;
          }

          return false;

        case IR_INST_IDIV:
          if (left->type != IR_OP_INT32 || right->type != IR_OP_INT32)
            return false;

          if (right->value.int32_value == 0)
            return false;  // TODO: optimize as throw

          *result = (IrOperand){IR_OP_INT32,
                                {.int32_value = left->value.int32_value /
                                                right->value.int32_value}};
          return true;

        case IR_INST_DIV:
          if (left->type != IR_OP_DOUBLE || right->type != IR_OP_DOUBLE)
            return false;

          if (right->value.double_value == 0)
            return false;  // TODO: optimize as throw

          *result = (IrOperand){IR_OP_DOUBLE,
                                {.double_value = left->value.double_value /
                                                 right->value.double_value}};
          return true;

        // Handle all comparison operations at once
        // 1. Calculate the "order" of the operands
        // 2. Based on the instruction type and the order, return true or false
        case IR_INST_EQ:
        case IR_INST_NE:
        case IR_INST_LT:
        case IR_INST_LE:
        case IR_INST_GT:
        case IR_INST_GE: {
          if (left->type != right->type)
            return false;  // TODO: optimize as throw

          int order = 0;

          // Calculate the order of the operands based on their type
          // (1 if left > right, 0 if equal, -1 if left < right)
          switch (left->type) {
            case IR_OP_INT32:
              order = (left->value.int32_value > right->value.int32_value) -
                      (left->value.int32_value < right->value.int32_value);
              break;

            case IR_OP_DOUBLE:
              order = (left->value.double_value > right->value.double_value) -
                      (left->value.double_value < right->value.double_value);
              break;

            case IR_OP_STRING:
              order =
                  strcmp(left->value.string_value, right->value.string_value);
              break;

            case IR_OP_BOOL:
              if (instruction->type != IR_INST_EQ &&
                  instruction->type != IR_INST_NE)
                return false;  // TODO: check spec

              order = left->value.bool_value - right->value.bool_value;
              break;

            default:
              return false;
          }

          bool value = false;

          // Fold the comparison based on the instruction type and the order of
          // the operands
          switch (instruction->type) {
            case IR_INST_EQ:
              value = order == 0;
              break;
            case IR_INST_NE:
              value = order != 0;
              break;
            case IR_INST_LT:
              value = order < 0;
              break;
            case IR_INST_LE:
              value = order <= 0;
              break;
            case IR_INST_GT:
              value = order > 0;
              break;
            case IR_INST_GE:
              value = order >= 0;
              break;

            default:
              return false;
          }

          *result = (IrOperand){IR_OP_BOOL, {.bool_value = value}};
          return true;
        }

        case IR_INST_CONCAT: {
          if (left->type != IR_OP_STRING || right->type != IR_OP_STRING)
            return false;

          size_t left_len = strlen(left->value.string_value);
          size_t right_len = strlen(right->value.string_value);

          // Allocate memory for the concatenated string
          char* concat = TRY_MALLOC(left_len + right_len + 1);

          // Copy the string values into the concatenated string
          memcpy(concat, left->value.string_value, left_len);
          memcpy(concat + left_len, right->value.string_value, right_len);
          concat[left_len + right_len] = '\0';

          *result = (IrOperand){IR_OP_STRING, {.string_value = concat}};

          return true;
        }

        default:
          return false;
      }
    } break;

    // Function calls
    case IR_INST_CLASS_CALL:
      // TODO: optimize pure function calls with constant arguments
      return false;  // to stop linter from complaining

    // Store and load remain and they cant be folded
    default:
      break;
  }

  return false;
}

bool optimizer_fold_function(Ctx* ctx, IrFunctionId function_id) {
  IrFunction* function = &ctx->ir.functions.data[function_id];

  bool modified = false;

  // Iterate over all blocks in the function
  for (IrBlockId bid = 0; bid < function->blocks.count; bid++) {
    IrBlock* block = &function->blocks.data[bid];

    // Iterate over all instructions in the block
    for (size_t instId = 0; instId < block->instructions.count; instId++) {
      IrInstruction* instruction = &block->instructions.data[instId];

      IrOperand result;

      // Try to fold the instruction
      if (!optimizer_fold_instruction(ctx, instruction, &result))
        continue;  // Unable to fold, go to next instruction

      // Replace the value with the constant
      size_t replacements = optimizer_replace_value_with_constant(
          ctx, function_id, instruction->result, result, true);

      // If the result is a string and it's not used anywhere, free it
      if (result.type == IR_OP_STRING && replacements == 0)
        free(result.value.string_value);

      // The instruction was folded, remove it as it's no longer needed
      ir_free_instruction(instruction);
      array_remove(ctx, &block->instructions, instId);

      // Decrement the instruction index to account for the removed instruction
      instId--;

      // Mark the function as modified
      modified = true;
    }
  }

  // Return whether the function was modified
  return modified;
}
