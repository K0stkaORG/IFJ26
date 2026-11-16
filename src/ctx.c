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
#include "array.h"
#include "ast.h"  // ast_node_destroy
#include "ir.h"   // ir_init, ir_free
#include "lexer.h"
#include "optimizer.h"
#include "symtable.h"           // symtable_init, symtable_destroy
#include "symtable_builtins.h"  // symtable_builtins_populate

#include <errno.h>   // errno, ERANGE
#include <limits.h>  // INT_MAX
#include <setjmp.h>  // longjmp
#include <stdarg.h>  // va_list, va_start, va_end
#include <stdio.h>   // fprintf, size_t, vfprintf, stderr
#include <stdlib.h>  // malloc, realloc, strtol, exit
#include <string.h>  // strcmp

void ctx_init(Ctx* ctx, Flags flags) {
  // Default exit code
  ctx->exit_code = E_OK;

  // Default error handling scope
  ctx->error_frame = NULL;

  // Flags
  ctx->flags = flags;

  // Initialize AST
  ctx->ast = NULL;

  // Initialize global symtable
  symtable_init(ctx, &ctx->global_symtable, NULL);

  // Initialize lexer struct
  array_init(&ctx->lexer.buffer);
  array_init(&ctx->lexer.stack);
  array_init(&ctx->lexer.saved_stack);
  ctx->lexer.line = 1;
  ctx->lexer.column = 1;
  ctx->lexer.cursor = 0;
  ctx->lexer.indent_flag = false;

  // Initialize IR program
  ir_init(&ctx->ir);

  // Initialize optimizer context
  optimizer_init(&ctx->optimizer);
}

void ctx_allocate(Ctx* ctx) {
  // Populate global symbol table with IFJ26 built-in functions
  symtable_builtins_populate(ctx, &ctx->global_symtable);
}

bool parse_cli_args(int argc, char* argv[], Flags* flags) {
  *flags = DEFAULT_FLAGS;

  int current = 1;
  // For all CLI arguments
  while (current < argc) {
    // Handle --debugForceNthMallocFail
    if (strcmp(argv[current], "--debugForceNthMallocFail") == 0) {
      // Check that there is a value for the flag
      if (current + 1 >= argc)
        return false;

      // Parse the value of the flag
      errno = 0;
      char* end = NULL;
      long value = strtol(argv[current + 1], &end, 10);

      // Check that the value is a valid, positive integer
      if (errno == ERANGE || *end != '\0' || value < 1)
        return false;

      flags->forceNthMallocFail = (int)value;

      // Consume both arguments
      current += 2;
      continue;
    }

    // Handle --verbose
    if (strcmp(argv[current], "--verbose") == 0) {
      flags->verbose = true;

      // Consume the argument
      current += 1;
      continue;
    }

    // Handle -o0
    if (strcmp(argv[current], "-o0") == 0) {
      flags->disableOptimizations = true;

      // Consume the argument
      current += 1;
      continue;
    }

    // Unrecognized argument
    return false;
  }

  // All arguments were parsed successfully
  return true;
}

ExitCode ctx_free(Ctx* ctx) {
  // Free allocated memory
  array_free(&ctx->lexer.buffer);
  array_free(&ctx->lexer.stack);
  array_free(&ctx->lexer.saved_stack);
  ir_free(&ctx->ir);

  // Free the AST
  if (ctx->ast != NULL) {
    AstNode* ast = ctx->ast;
    ctx->ast = NULL;
    ast_node_destroy(ctx, ast);
  }

  // Destroy global symbol table
  symtable_destroy(&ctx->global_symtable);

  // Free intermediate representation
  ir_free(&ctx->ir);

  // Free optimizer context
  optimizer_free(&ctx->optimizer);

  // Return the exit code
  return ctx->exit_code;
}

/**
 * @brief Returns whether the next malloc should fail and updates the context
 *
 * @param[in,out] ctx The context to check
 * @return Whether the next malloc should fail
 */
static bool should_fail_malloc(Ctx* ctx) {
  if (ctx->flags.forceNthMallocFail == 0)
    return false;

  ctx->flags.forceNthMallocFail--;
  return ctx->flags.forceNthMallocFail == 0;
}

void* ctx_malloc_helper(Ctx* ctx,
                        size_t size,
                        const char* file,
                        int line,
                        bool shouldJump) {
  void* ptr;

  // If malloc should fail, force a NULL pointer
  bool forcedFailure = should_fail_malloc(ctx);
  if (forcedFailure)
    ptr = NULL;

  // Otherwise, attempt to allocate memory
  else
    ptr = malloc(size);

  // On success, return the allocated pointer
  if (ptr != NULL)
    return ptr;

  // Otherwise throw an error
  if (forcedFailure)
    throw_error_helper(
        ctx, E_INTERNAL_ERROR, file, line, shouldJump,
        "Failed to allocate %zu B of memory (forced allocation failure)", size);
  else
    throw_error_helper(ctx, E_INTERNAL_ERROR, file, line, shouldJump,
                       "Failed to allocate %zu B of memory", size);

  // If should not jump, return NULL
  return NULL;
}

void* ctx_realloc_helper(Ctx* ctx,
                         void* ptr,
                         size_t size,
                         const char* file,
                         int line) {
  void* newPtr;

  // If malloc should fail, force a NULL pointer
  bool forcedFailure = should_fail_malloc(ctx);
  if (forcedFailure)
    newPtr = NULL;

  // Otherwise, attempt to reallocate memory
  else
    newPtr = realloc(ptr, size);

  // On success, return the reallocated pointer
  if (newPtr != NULL)
    return newPtr;

  // Otherwise, throw an error and long jump to the error target
  if (forcedFailure)
    throw_error_helper(
        ctx, E_INTERNAL_ERROR, file, line, true,
        "Failed to reallocate %zu B of memory (forced allocation failure)",
        size);
  else
    throw_error_helper(ctx, E_INTERNAL_ERROR, file, line, true,
                       "Failed to reallocate %zu B of memory", size);

  return ptr;  // unreachable
}

// AI generated by: Codex / gpt-6.2-sol
// Prompt: "From b822a8f71c26d360d764c3676474501e037ed472 pull the TRY/CATCH
// feature + tests to the current working directory"
// Changes: Dispatch errors to the scoped handler or root target.
_Noreturn void ctx_rethrow(Ctx* ctx) {
  CtxErrorFrame* frame = ctx->error_frame;

  // If we have an active error frame, pop it and jump to its target
  if (frame != NULL) {
    // Restore the parent before CATCH executes, allowing a throw from CATCH
    ctx->error_frame = frame->previous;
    longjmp(frame->target, 1);
  }

  // AI generated by: Codex / gpt-6.2-sol
  // Prompt: "Migrate tests to the new error handling and in case there is no
  // error scope, exit with E_INTERNAL_ERROR and print a message to stderr"
  // Changes: Report and exit on an unhandled compiler error.
  fprintf(stderr, "Fatal: Error thrown outside of a TRY scope\n");
  exit(E_INTERNAL_ERROR);
}

void throw_error_helper(Ctx* ctx,
                        ExitCode code,
                        const char* file,
                        int line,
                        bool shouldJump,
                        const char* format,
                        ...) {
  // Update the exit code
  ctx->exit_code = code;

  // Print the error message to stderr
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  fprintf(stderr, "\n    thrown at %s:%d, code: %d\n", file, line, code);
  va_end(args);

  // If shouldJump is true, jump to the nearest TRY scope
  if (shouldJump)
    ctx_rethrow(ctx);
}

void debug_print_helper(Ctx* ctx,
                        const char* file,
                        int line,
                        const char* format,
                        ...) {
  // Check if debug printing is enabled
  if (!ctx->flags.verbose)
    return;

  // Print the debug message to stderr
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  fprintf(stderr, "\n    source: %s:%d\n", file, line);
  va_end(args);
}
