/**
 * @file ir_free_helpers.h
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 12. 11. 2026
 *
 * @brief Intermediate representation helper cleanup functions
 *
 * This file contains definitions of helper functions for freeing memory used by
 * intermediate representation components.
 */

#ifndef IR_FREE_HELPERS_H
#define IR_FREE_HELPERS_H

#include "ir.h"

/**
 * @brief Frees the memory used by an operand
 *
 * Does not free the operand pointer itself. Used internally by IR cleanup
 * and construction helpers.
 *
 * @param[in,out] operand The operand to free
 */
void ir_free_operand(IrOperand* operand);

/**
 * @brief Frees the memory used by an instruction.
 *
 * Frees any memory used by an instruction. It is safe to call this function on
 * a already/partially freed instruction. Does not free the instruction pointer
 * itself.
 *
 * @param[in,out] instruction The instruction to free.
 */
void ir_free_instruction(IrInstruction* instruction);

/**
 * @brief Frees the memory used by a block.
 *
 * Frees any memory used by a block. It is safe to call this function on a
 * already/partially freed block. Does not free the block pointer itself.
 *
 * @param[in,out] block The block to free.
 */
void ir_free_block(IrBlock* block);

/**
 * @brief Frees the memory used by a function.
 *
 * Frees any memory used by a function. It is safe to call this function on a
 * already/partially freed function. Does not free the function pointer itself.
 *
 * @param[in,out] function The function to free.
 */
void ir_free_function(IrFunction* function);

#endif
