/**
 * @file optimizer_forwarding.h
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 16. 11. 2026
 *
 * @brief Helpers for store-load forwarding
 *
 * Contains declarations for helper functions for store-load forwarding.
 */

#ifndef OPTIMIZER_FORWARDING_H
#define OPTIMIZER_FORWARDING_H

#include "ctx.h"
#include "ir.h"

#include <stdbool.h>  // bool

/**
 * @brief Forward stores to loads in the given function
 *
 * @param ctx The context
 * @param function_id The function to forward stores in
 * @return bool true if the function was modified, false otherwise
 */
bool optimizer_forward_stores(Ctx* ctx, IrFunctionId function_id);

#endif
