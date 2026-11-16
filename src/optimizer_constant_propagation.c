/**
 * @file optimizer_constant_propagation.c
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 12. 11. 2026
 *
 * @brief Helpers for propagating constants in the IR
 *
 * This file contains definitions for helper functions for propagating constants
 * in the IR.
 */

#include "optimizer_constant_propagation.h"

#include "ctx.h"
#include "ir.h"
#include "ir_helpers.h"

#include <stdbool.h>  // bool
#include <stddef.h>   // size_t

/**
 * @brief Replaces an operand in the IR
 *
 * @param[in,out] ctx The context of the compiler
 * @param[out] operand The operand to replace
 * @param[in] value_id The id of the value to replace
 * @param[in] replacement The replacement operand
 * @param[in] move_ownership Whether to move the ownership of the string value
 * @param[in,out] ownership_taken Whether the ownership of the string value has
 * been taken
 * @return 1 if the operand was replaced, 0 otherwise
 */
static size_t replace_operand(Ctx* ctx,
                              IrOperand* operand,
                              IrValueId value_id,
                              IrOperand* replacement,
                              bool move_ownership,
                              bool* ownership_taken) {
  // Check if the operand is a value and has the correct id
  if (operand->type != IR_OP_VALUE || operand->value.id != value_id)
    return 0;

  switch (replacement->type) {
    case IR_OP_STRING:
      // Check if we can simply move the ownership of the string value
      if (move_ownership && !*ownership_taken) {
        *operand = *replacement;
        *ownership_taken = true;

        // Otherwise, deep copy the string value
      } else {
        char* copy = copy_string(ctx, replacement->value.string_value);

        operand->type = IR_OP_STRING;
        operand->value.string_value = copy;
      }
      break;

    // For non-string constant types, simply copy the value
    default:
      *operand = *replacement;
      break;
  }

  return 1;
}

size_t optimizer_replace_value(Ctx* ctx,
                               IrFunctionId function_id,
                               IrValueId value_id,
                               IrOperand constant,
                               bool move_ownership) {
  IrFunction* function = &ctx->ir.functions.data[function_id];

  size_t replacements = 0;
  bool ownership_taken = false;

  // Go through each block
  for (IrBlockId bid = 0; bid < function->blocks.count; bid++) {
    IrBlock* block = &function->blocks.data[bid];

    // And replace instruction operands ...
    for (size_t inst_id = 0; inst_id < block->instructions.count; inst_id++) {
      IrInstruction* instruction = &block->instructions.data[inst_id];

      switch (ir_get_instruction_class(instruction)) {
        case IR_INST_CLASS_UNARY:
          replacements += replace_operand(
              ctx, &instruction->operands.unary.operand, value_id, &constant,
              move_ownership, &ownership_taken);
          break;

        case IR_INST_CLASS_BINARY:
          replacements +=
              replace_operand(ctx, &instruction->operands.binary.left, value_id,
                              &constant, move_ownership, &ownership_taken);
          replacements += replace_operand(
              ctx, &instruction->operands.binary.right, value_id, &constant,
              move_ownership, &ownership_taken);
          break;

        case IR_INST_CLASS_STORE:
          replacements += replace_operand(
              ctx, &instruction->operands.store.source, value_id, &constant,
              move_ownership, &ownership_taken);
          break;

        case IR_INST_CLASS_CALL:
          for (size_t op_id = 0; op_id < instruction->operands.call.arg_count;
               op_id++)
            replacements += replace_operand(
                ctx, &instruction->operands.call.args[op_id], value_id,
                &constant, move_ownership, &ownership_taken);
          break;

        // Only load remains and that has no operands
        default:
          break;
      }
    }

    // ... and terminator operands
    IrBlockTerminator* terminator = &block->terminator;
    IrOperand* operand = NULL;

    switch (terminator->type) {
      case IR_TERMINATOR_CONDITIONAL:
        operand = &terminator->value.conditional_target.condition;
        break;
      case IR_TERMINATOR_RETURN:
        operand = &terminator->value.return_value;
        break;
      case IR_TERMINATOR_EXIT:
        operand = &terminator->value.exit_code;
        break;
      default:
        break;  // JUMP has no operand
    }

    if (operand != NULL)
      replacements += replace_operand(ctx, operand, value_id, &constant,
                                      move_ownership, &ownership_taken);
  }

  return replacements;
}
