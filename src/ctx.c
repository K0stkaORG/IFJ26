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
#include <stdlib.h>  // malloc

void ctx_init(Ctx* ctx) {
  ctx->exit_code = E_OK;

  ctx->flags.forceMallocFail = false;

  ctx->buffer = NULL;
}

void ctx_free(Ctx* ctx) {
  free(ctx->buffer);
  ctx->buffer = NULL;
}

void* ctx_malloc_helper(Ctx* ctx, size_t size, const char* file, int line) {
  void* ptr;

  // If malloc should fail, force a NULL pointer
  if (ctx->flags.forceMallocFail)
    ptr = NULL;

  // Otherwise, attempt to allocate memory
  else
    ptr = malloc(size);

  // On success, return the allocated pointer
  if (ptr != NULL)
    return ptr;

  // Otherwise throw an error
  throw_error_helper(ctx, E_INTERNAL_ERROR, file, line,
                     "Failed to allocate %zu B of memory", size);
  return NULL;
}

void* ctx_realloc_helper(Ctx* ctx,
                         void* ptr,
                         size_t size,
                         const char* file,
                         int line) {
  void* newPtr;

  // If malloc should fail, force a NULL pointer
  if (ctx->flags.forceMallocFail)
    newPtr = NULL;

  // Otherwise, attempt to reallocate memory
  else
    newPtr = realloc(ptr, size);

  // On success, return the reallocated pointer
  if (newPtr != NULL)
    return newPtr;

  // Otherwise, throw an error and return the original pointer
  throw_error_helper(ctx, E_INTERNAL_ERROR, file, line,
                     "Failed to reallocate %zu B of memory", size);
  return ptr;
}

void throw_error_helper(Ctx* ctx,
                        ExitCode code,
                        const char* file,
                        int line,
                        const char* format,
                        ...) {
  /* If the code is more specific than the current one, replace it */
  if (ctx->exit_code == E_OK || (ctx->exit_code == E_SEMANTIC_OTHER_ERROR &&
                                 (code) >= E_SEMANTIC_UNDEFINED_ERROR &&
                                 (code) <= E_SEMANTIC_REWRITE_ERROR))
    ctx->exit_code = code;

  /* Print the error message to stderr regardless */
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  fprintf(stderr, "\n    thrown at %s:%d, code: %d\n", file, line, code);
  va_end(args);
}
