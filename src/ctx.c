/**
 * @file ctx.c
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 9. 10. 2026
 *
 * @brief Shared memory and error context helper functions
 *
 * This file contains helper functions for working with the shared context,
 * unified resource allocation and freeing.
 */

#include "ctx.h"

void ctx_init(Ctx* ctx) {
  ctx->exit_code = E_OK;
}
