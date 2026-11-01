/**
 * @file ir.c
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 1. 11. 2026
 *
 * @brief Intermediate representation helper functions
 *
 * This file contains helper functions for working with the intermediate
 * representation of the program.
 */

#include "ir.h"

#include <stdlib.h>  // free

void ir_init(IrProgram* ir) {
  // Set IR to default values
  ir->entrypoint = IR_INVALID_ID;
  ir->functions = NULL;
  ir->function_count = 0;
}

/**
 * @brief Frees the memory used by an instruction.
 *
 * Frees any memory used by an instruction. It is safe to call this function on
 * a already/partially freed instruction
 *
 * @param[in,out] instruction The instruction to free.
 */
static void ir_free_instruction(IrInstruction* instruction) {
  // Check the pointer is valid
  if (instruction == NULL)
    return;

  // Free the operands (if dynamically allocated)
  if (instruction->type == IR_INST_CALL)
    if (instruction->operands.call.args != NULL)
      free(instruction->operands.call.args);

  // Free the instruction
  free(instruction);
}

/**
 * @brief Frees the memory used by a block.
 *
 * Frees any memory used by a block. It is safe to call this function on a
 * already/partially freed block
 *
 * @param[in,out] block The block to free.
 */
static void ir_free_block(IrBlock* block) {
  // Check the pointer is valid
  if (block == NULL)
    return;

  // Free instructions
  if (block->instructions != NULL) {
    // Free each instruction
    for (int instId = 0; instId < block->instruction_count; instId++)
      ir_free_instruction(&block->instructions[instId]);

    // Free the instruction array
    free(block->instructions);
  }

  // Free the block
  free(block);
}

/**
 * @brief Frees the memory used by a function.
 *
 * Frees any memory used by a function. It is safe to call this function on a
 * already/partially freed function
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
  if (function->slots != NULL) {
    // Free slot names
    for (int sid = 0; sid < function->slot_count; sid++)
      if (function->slots[sid].name != NULL)
        free(function->slots[sid].name);

    // Free slot array
    free(function->slots);
  }

  // Free function blocks
  if (function->blocks != NULL) {
    // Free each block
    for (int bid = 0; bid < function->block_count; bid++)
      ir_free_block(&function->blocks[bid]);

    // Free block array
    free(function->blocks);
  }

  // Free the function
  free(function);
}

void ir_free(IrProgram* ir) {
  // Free contents of each function
  for (int fid = 0; fid < ir->function_count; fid++)
    ir_free_function(&ir->functions[fid]);

  // Free functions array
  free(ir->functions);
  ir->functions = NULL;
  ir->function_count = 0;

  // Reset entrypoint
  ir->entrypoint = IR_INVALID_ID;
}
