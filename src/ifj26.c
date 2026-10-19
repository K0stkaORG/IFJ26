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
 */

#include "ctx.h"  // Shared memory and error context

/**
 * @brief Entrypoint of the IFJ26 compiler
 */
int main(void) {
  // Initialize shared memory and error context
  Ctx context;
  ctx_init(&context);

  // Step 2: Process input

  // Step 3: Lexer

  // Step 4: Parser

  // Step 5: Lowering to IR

  // Step 6: Optimizer

  // Step 7: Generate IFJ26Code

  // Step 8: Output and cleanup
  ctx_free(&context);

  return 0;
}
