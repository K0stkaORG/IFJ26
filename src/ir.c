/**
 * @file ir.c
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 1. 11. 2026
 *
 * @brief Intermediate representation initialization and cleanup
 *
 * This file contains functions for initializing and cleaning up the
 * intermediate representation of the program.
 */

#include "ir.h"
#include "array.h"
#include "ir_internal.h"

#include <stdlib.h>  // free

void ir_init(IrProgram* ir) {
  // Set IR to default values
  ir->entrypoint = IR_INVALID_ID;
  array_init(&ir->functions);
}

/**
 * @brief Frees the memory used by an operand.
 *
 * Frees any memory used by an operand. It is safe to call this function on
 * a already/partially freed operand. Does not free the operand pointer itself.
 *
 * @param[in,out] operand The operand to free.
 */
void ir_free_operand(IrOperand* operand) {
  // Check the pointer is valid
  if (operand == NULL)
    return;

  // Free the operand value (if dynamically allocated)
  if (operand->type == IR_OP_STRING)
    free(operand->value.string_value);
}

/**
 * @brief Frees the memory used by an instruction.
 *
 * Frees any memory used by an instruction. It is safe to call this function on
 * a already/partially freed instruction. Does not free the instruction pointer
 * itself.
 *
 * @param[in,out] instruction The instruction to free.
 */
static void ir_free_instruction(IrInstruction* instruction) {
  // Check the pointer is valid
  if (instruction == NULL)
    return;

  // Free the operands
  switch (instruction->type) {
    // Unary instructions
    case IR_INST_FLOOR:
    case IR_INST_NOT:
    case IR_INST_INT2DOUBLE:
      ir_free_operand(&instruction->operands.unary.operand);
      break;

    // Binary instructions
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
      ir_free_operand(&instruction->operands.binary.left);
      ir_free_operand(&instruction->operands.binary.right);
      break;

    // Store instruction
    case IR_INST_STORE:
      ir_free_operand(&instruction->operands.store.source);
      break;

    // Call instruction
    case IR_INST_CALL:
      if (instruction->operands.call.args != NULL) {
        // Free each argument
        for (size_t oid = 0; oid < instruction->operands.call.arg_count; oid++)
          ir_free_operand(&instruction->operands.call.args[oid]);

        // Free the argument array
        free(instruction->operands.call.args);
      }
      break;

    default:
      break;
  }
}

/**
 * @brief Frees the memory used by a block.
 *
 * Frees any memory used by a block. It is safe to call this function on a
 * already/partially freed block. Does not free the block pointer itself.
 *
 * @param[in,out] block The block to free.
 */
static void ir_free_block(IrBlock* block) {
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

/**
 * @brief Frees the memory used by a function.
 *
 * Frees any memory used by a function. It is safe to call this function on a
 * already/partially freed function. Does not free the function pointer itself.
 *
 * @param[in,out] function The function to free.
 */
static void ir_free_function(IrFunction* function) {
  // Check the pointer is valid
  if (function == NULL)
    return;

  // If allocated, free function name
  if (function->name != NULL)
    free(function->name);

  // If allocated, free function slots
  if (function->slots.data != NULL) {
    // Free slot names
    for (IrStackSlotId sid = 0; sid < function->slots.count; sid++)
      if (function->slots.data[sid].name != NULL)
        free(function->slots.data[sid].name);

    // Free slot array
    array_free(&function->slots);
  }

  // Free function blocks
  if (function->blocks.data != NULL) {
    // Free each block
    for (IrBlockId bid = 0; bid < function->blocks.count; bid++)
      ir_free_block(&function->blocks.data[bid]);

    // Free block array
    array_free(&function->blocks);
  }
}

void ir_free(IrProgram* ir) {
  // Check the pointer is valid
  if (ir->functions.data == NULL)
    return;

  // Free contents of each function
  for (IrFunctionId fid = 0; fid < ir->functions.count; fid++)
    ir_free_function(&ir->functions.data[fid]);

  // Free functions array
  array_free(&ir->functions);

  // Reset entrypoint
  ir->entrypoint = IR_INVALID_ID;
}
