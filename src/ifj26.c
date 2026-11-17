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

#include "codegen.h"    // cg_generate
#include "ctx.h"        // Shared memory and error context
#include "parser.h"     // parser_parse
#include "semantics.h"  // semantics_analyze

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
  Ctx* ctx = &context;
  ctx_init(ctx, flags);

  // For unified error handling, wrap the compilation process in a TRY block
  TRY {
    // Allocate memory for the shared context
    ctx_allocate(ctx);

    // Run the compiler
    parser_parse(ctx);
    semantics_analyze(ctx);
  }
  CATCH {
    // No need to do anything special in case of error, ctx_free will handle it
  }

  // Codegen
  cg_generate(stdout, &context);

  // Free allocated resources and exit
  return ctx_free(ctx);
}
