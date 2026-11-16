/**
 * @file optimizer_control_flow.h
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 16. 11. 2026
 *
 * @brief Helpers for control flow analysis and optimization
 *
 * Contains declarations for helper functions for control flow analysis and
 * optimization.
 */

#ifndef OPTIMIZER_CONTROL_FLOW_H
#define OPTIMIZER_CONTROL_FLOW_H

#include "ctx.h"
#include "ir.h"

#include <stdbool.h>  // bool

/**
 * @brief Removes unreachable blocks from the control flow graph
 *
 * @param ctx The context of the program
 * @param function_id The ID of the function to optimize
 * @return true if the function was modified, false otherwise
 */
bool optimizer_remove_unreachable_blocks(Ctx* ctx, IrFunctionId function_id);

#endif
