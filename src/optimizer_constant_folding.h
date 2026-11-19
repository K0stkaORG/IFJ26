/**
 * @file optimizer_constant_folding.h
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 12. 11. 2026
 *
 * @brief Helpers for folding constants in the IR
 *
 * This file contains declarations for helper functions for folding constants in
 * the IR.
 */

#ifndef OPTIMIZER_CONSTANT_FOLDING_H
#define OPTIMIZER_CONSTANT_FOLDING_H

#include "ctx.h"
#include "ir.h"

#include <stdbool.h>  // bool
#include <stddef.h>   // size_t

/**
 * @brief Folds constants in the given function.
 *
 * @param ctx The context.
 * @param function_id The ID of the function to fold constants in.
 * @return true if the function was modified, false otherwise.
 */
bool optimizer_fold_function(Ctx* ctx, IrFunctionId function_id);

#endif
