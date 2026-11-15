/**
 * @file optimizer_constant_propagation.h
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 12. 11. 2026
 *
 * @brief Helpers for propagating constants in the IR
 *
 * This file contains declarations for helper functions for propagating
 * constants in the IR.
 */

#ifndef OPTIMIZER_CONSTANT_PROPAGATION_H
#define OPTIMIZER_CONSTANT_PROPAGATION_H

#include "ctx.h"
#include "ir.h"

#include <stdbool.h>  // bool
#include <stddef.h>   // size_t

/**
 * @brief Replaces all occurrences of a value in the IR
 *
 * @param[in,out] ctx The context
 * @param[in] function_id The function ID
 * @param[in] value_id The value ID
 * @param[in] replacement The replacement value
 * @param[in] move_ownership Whether to move ownership of the replacement
 * @return The number of replacements made
 */
size_t optimizer_replace_value(Ctx* ctx,
                               IrFunctionId function_id,
                               IrValueId value_id,
                               IrOperand replacement,
                               bool move_ownership);

#endif
