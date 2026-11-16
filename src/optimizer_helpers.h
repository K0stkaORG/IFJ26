/**
 * @file optimizer_helpers.h
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 16. 11. 2026
 *
 * @brief Helpers for optimizing the IR
 *
 * Contains declarations for helper functions for optimizing the IR.
 */

#ifndef OPTIMIZER_HELPERS_H
#define OPTIMIZER_HELPERS_H

#include "ir.h"

#include <stdbool.h>  // bool

/**
 * @brief Finds the definition of a value in the IR
 *
 * @param[in] function The function to search in
 * @param[in] value The value to search for
 * @return The instruction that defines the value, or NULL if not found
 */
IrInstruction* optimizer_find_definition(IrFunction* function, IrValueId value);

/**
 * @brief Returns whether the instruction is pure (does not have side effects)
 *
 * @param[in] type The type of the instruction
 * @return true if the instruction is pure, false otherwise
 */
bool optimizer_is_pure(IrInstructionType type);

/**
 * @brief Counts the number of uses of each value in the given function
 *
 * @param[in] function The function to search in
 * @param[out] uses The number of uses of each value
 */
void optimizer_count_value_uses(IrFunction* function, size_t* uses);

#endif
