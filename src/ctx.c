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

#include <stdarg.h>  // va_list, va_start, va_end
#include <stdio.h>   // fprintf, vfprintf, stderr

void ctx_init(Ctx* ctx) {
  ctx->exit_code = E_OK;
}

void throw_error_helper(Ctx* ctx, ExitCode code, const char* format, ...) {
  /* If the code is more specific than the current one, replace it */
  if (ctx->exit_code == E_OK || (ctx->exit_code == E_SEMANTIC_OTHER_ERROR &&
                                 (code) >= E_SEMANTIC_UNDEFINED_ERROR &&
                                 (code) <= E_SEMANTIC_REWRITE_ERROR))
    ctx->exit_code = code;

  /* Print the error message to stderr regardless */
  va_list args;
  va_start(args, format);
  fprintf(stderr, "Error %d: ", code);
  vfprintf(stderr, format, args);
  fprintf(stderr, "\n");
  va_end(args);
}
