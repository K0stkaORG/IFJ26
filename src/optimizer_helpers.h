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

/**
 * @brief Finds the definition of a value in the IR
 *
 * @param[in] function The function to search in
 * @param[in] value The value to search for
 * @return The instruction that defines the value, or NULL if not found
 */
IrInstruction* optimizer_find_definition(IrFunction* function, IrValueId value);

#endif
