/**
 * @file optimizer_dead_code_elimination.h
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 16. 11. 2026
 *
 * @brief Helpers for dead code elimination
 *
 * Contains declarations for helper functions for eliminating dead code.
 */

#ifndef OPTIMIZER_DEAD_CODE_ELIMINATION_H
#define OPTIMIZER_DEAD_CODE_ELIMINATION_H

#include "ctx.h"
#include "ir.h"

#include <stdbool.h>  // bool

/**
 * @brief Eliminates dead code from the function
 *
 * @param[in,out] ctx The context of the program
 * @param[in] function_id The id of the function to eliminate dead code from
 * @return true if the function was modified, false otherwise
 */
bool optimizer_eliminate_dead_code(Ctx* ctx, IrFunctionId function_id);

#endif
