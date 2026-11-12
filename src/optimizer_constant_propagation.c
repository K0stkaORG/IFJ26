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
 * @brief Replaces an operand with a constant in the IR
 *
 * @param[in,out] ctx The context of the compiler
 * @param[out] operand The operand to replace
 * @param[in] value_id The id of the value to replace
 * @param[in] constant The constant to replace with
 * @param[in] move_ownership Whether to move the ownership of the string value
 * @param[in,out] ownership_taken Whether the ownership of the string value has
 * been taken
 * @return 1 if the operand was replaced, 0 otherwise
 */
static size_t replace_operand_with_constant(Ctx* ctx,
                                            IrOperand* operand,
                                            IrValueId value_id,
                                            IrOperand* constant,
                                            bool move_ownership,
                                            bool* ownership_taken) {
  // Check if the operand is a value and has the correct id
  if (operand->type != IR_OP_VALUE || operand->value.id != value_id)
    return 0;

  switch (constant->type) {
    case IR_OP_STRING:
      // Check if we can simply move the ownership of the string value
      if (move_ownership && !*ownership_taken) {
        *operand = *constant;
        *ownership_taken = true;

        // Otherwise, deep copy the string value
      } else {
        char* copy = copy_string(ctx, constant->value.string_value);

        operand->type = IR_OP_STRING;
        operand->value.string_value = copy;
      }
      break;

    // For non-string constant types, simply copy the value
    case IR_OP_INT32:
    case IR_OP_DOUBLE:
    case IR_OP_BOOL:
    case IR_OP_UNIT:
      *operand = *constant;
      break;

    // IR_OP_VALUE is not a constant type, so it cannot be replaced
    default:
      THROW(E_INTERNAL_ERROR, "Cannot replace operand with constant of type %d",
            constant->type);
  }

  return 1;
}

size_t optimizer_replace_value_with_constant(Ctx* ctx,
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
    for (size_t instId = 0; instId < block->instructions.count; instId++) {
      IrInstruction* instruction = &block->instructions.data[instId];

      switch (instruction->type) {
        // Unary operations
        case IR_INST_NOT:
        case IR_INST_INT2DOUBLE:
        case IR_INST_FLOOR:
          replacements += replace_operand_with_constant(
              ctx, &instruction->operands.unary.operand, value_id, &constant,
              move_ownership, &ownership_taken);
          break;

        // Binary operations
        case IR_INST_ADD:
        case IR_INST_SUB:
        case IR_INST_MUL:
        case IR_INST_IDIV:
        case IR_INST_DIV:
        case IR_INST_EQ:
        case IR_INST_NE:
        case IR_INST_LT:
        case IR_INST_LE:
        case IR_INST_GT:
        case IR_INST_GE:
        case IR_INST_CONCAT:
          replacements += replace_operand_with_constant(
              ctx, &instruction->operands.binary.left, value_id, &constant,
              move_ownership, &ownership_taken);
          replacements += replace_operand_with_constant(
              ctx, &instruction->operands.binary.right, value_id, &constant,
              move_ownership, &ownership_taken);

        // Store
        case IR_INST_STORE:
          replacements += replace_operand_with_constant(
              ctx, &instruction->operands.store.source, value_id, &constant,
              move_ownership, &ownership_taken);

        // Function calls
        case IR_INST_CALL:
          for (size_t opId = 0; opId < instruction->operands.call.arg_count;
               opId++)
            replacements += replace_operand_with_constant(
                ctx, &instruction->operands.call.args[opId], value_id,
                &constant, move_ownership, &ownership_taken);

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
      replacements += replace_operand_with_constant(
          ctx, operand, value_id, &constant, move_ownership, &ownership_taken);
  }

  return replacements;
}
