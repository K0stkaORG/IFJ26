/**
 * @file ifj26.c
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 4. 10. 2026
 *
 * @brief Main entrypoint of the IFJ26 compiler
 *
 * This file contains the main entrypoint of the IFJ26 compiler, handles I/O,
 * errors, shared memory context and orchestrates the compilation process.
 *
 * Usage:
 *  ifj26 [--debugForceNthMallocFail <n>] [--verbose] [-o0]
 */

#include <stdio.h>  // fprintf, stderr

#include "codegen.h"  // cg_generate
#include "ctx.h"      // Shared memory and error context

/**
 * @brief Entrypoint of the IFJ26 compiler
 */
int main(int argc, char** argv) {
  // Parse CLI flags
  Flags flags;
  if (!parse_cli_args(argc, argv, &flags)) {
    fprintf(stderr, "Invalid CLI flag provided\n");
    return E_INTERNAL_ERROR;
  }

  // Initialize shared memory and error context
  static Ctx context;  // Static to survive long jump
  ctx_init(&context, flags);

  // If an error is thrown at any point during the program execution, an error
  // message will be printed to stderr and the CPU will jump back to here. If we
  // got here because of a thrown error, the jump_result will be 1 and we will
  // clean up and exit with the thrown error code.
  int jump_result = setjmp(context.error_target);
  if (jump_result != 0)
    return ctx_free(&context);

  // Allocate memory for the shared context
  ctx_allocate(&context);

  // Run the compiler

  // Codegen
  cg_generate(stdout, &context);

  // Free allocated resources and exit
  return ctx_free(&context);
}
