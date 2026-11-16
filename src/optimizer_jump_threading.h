/**
 * @file optimizer_jump_threading.h
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 16. 11. 2026
 *
 * @brief Helpers for jump threading optimization
 *
 * Contains declarations for helper functions for control flow analysis and
 * optimization.
 */

#ifndef OPTIMIZER_JUMP_THREADING_H
#define OPTIMIZER_JUMP_THREADING_H

#include "ctx.h"
#include "ir.h"

#include <stdbool.h>  // bool

/**
 * @brief Simplifies jumps to blocks with no instructions and jump terminator
 *
 * @param[in] ctx The context of the program
 * @param[in] fid Id of the function to optimize
 * @return true if the function was modified, false otherwise
 */
bool optimizer_simplify_jumps(Ctx* ctx, IrFunctionId fid);

#endif
