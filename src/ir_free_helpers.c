/**
 * @file ir_free_helpers.c
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 12. 11. 2026
 *
 * @brief Intermediate representation helper cleanup functions
 *
 * This file contains functions for cleaning up parts of the intermediate
 * representation of the program.
 */

#include "ir_free_helpers.h"

#include "array.h"
#include "ir.h"
#include "ir_helpers.h"

#include <stdlib.h>  // free

void ir_free_operand(IrOperand* operand) {
  // Check the pointer is valid
  if (operand == NULL)
    return;

  // Free the operand value (if dynamically allocated)
  if (operand->type == IR_OP_STRING)
    free(operand->value.string_value);
}

void ir_free_instruction(IrInstruction* instruction) {
  // Check the pointer is valid
  if (instruction == NULL)
    return;

  // Free the operands
  switch (ir_get_instruction_class(instruction)) {
    case IR_INST_CLASS_UNARY:
      ir_free_operand(&instruction->operands.unary.operand);
      break;

    case IR_INST_CLASS_BINARY:
      ir_free_operand(&instruction->operands.binary.left);
      ir_free_operand(&instruction->operands.binary.right);
      break;

    case IR_INST_CLASS_STORE:
      ir_free_operand(&instruction->operands.store.source);
      break;

    case IR_INST_CLASS_CALL:
      if (instruction->operands.call.args != NULL) {
        // Free each argument
        for (size_t oid = 0; oid < instruction->operands.call.arg_count; oid++)
          ir_free_operand(&instruction->operands.call.args[oid]);

        // Free the argument array
        free(instruction->operands.call.args);
      }
      break;

    // Only LOAD remains and it does not have any operands
    default:
      break;
  }
}

void ir_free_block(IrBlock* block) {
  // Check the pointer is valid
  if (block == NULL)
    return;

  // Free instructions
  if (block->instructions.data != NULL) {
    // Free each instruction
    for (size_t instId = 0; instId < block->instructions.count; instId++)
      ir_free_instruction(&block->instructions.data[instId]);

    // Free the instruction array
    array_free(&block->instructions);
  }

  // Free the terminator
  switch (block->terminator.type) {
    case IR_TERMINATOR_CONDITIONAL:
      ir_free_operand(&block->terminator.value.conditional_target.condition);
      break;

    case IR_TERMINATOR_RETURN:
      ir_free_operand(&block->terminator.value.return_value);
      break;

    case IR_TERMINATOR_EXIT:
      ir_free_operand(&block->terminator.value.exit_code);
      break;

    default:
      break;
  }
}

void ir_free_function(IrFunction* function) {
  // Check the pointer is valid
  if (function == NULL)
    return;

  // If allocated, free function name
  if (function->name != NULL)
    free(function->name);

  // If allocated, free function slots
  if (function->slots.data != NULL) {
    // Free slot names
    for (size_t sid = 0; sid < function->slots.count; sid++)
      if (function->slots.data[sid].name != NULL)
        free(function->slots.data[sid].name);

    // Free slot array
    array_free(&function->slots);
  }

  // Free function blocks
  if (function->blocks.data != NULL) {
    // Free each block
    for (size_t bid = 0; bid < function->blocks.count; bid++)
      ir_free_block(&function->blocks.data[bid]);

    // Free block array
    array_free(&function->blocks);
  }
}
