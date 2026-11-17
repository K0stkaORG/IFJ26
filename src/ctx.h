/**
 * @file ctx.h
 * Project: IFJ26
 *
 * @author Jan Kostka (xkostkj00)
 * @date 9. 10. 2026
 *
 * @brief Shared memory and error context for the IFJ26 compiler
 *
 * This file contains the shape of the shared memory and error context for the
 * IFJ26 compiler. It also contains macros for simple and unified error
 * handling.
 */

#ifndef CTX_H
#define CTX_H

#include "ir.h"        // IrProgram
#include "lexer.h"     // LexerByteBuffer, LexerStack
#include "symtable.h"  // Symtable

#include <setjmp.h>   // jmp_buf
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL, size_t

typedef struct AstNode AstNode;  // Forward declaration (ast.h includes ctx.h)
typedef struct Parser Parser;    // Forward declaration (parser state)

/**
 * @brief Exit codes for the IFJ26 compiler
 */
typedef enum {
  // Success
  E_OK = 0,

  // "Chyba v programu v rámci lexikální analýzy (chybná struktura aktuálního
  // lexému, komentáře nebo odsazení)"
  E_LEXER_ERROR = 1,

  // "Chyba v programu v rámci syntaktické analýzy (chybná syntaxe, chybějící
  // prolog, nepovolená konstrukce nebo porušení struktury odsazených bloků)"
  E_SYNTAX_ERROR = 2,

  // "Semantická chyba – použití nedefinované funkce, proměnné nebo typu;
  // chybějící funkce s anotací @main"
  E_SEMANTIC_UNDEFINED_ERROR = 3,

  // "Semantická chyba – nepovolená redefinice funkce, proměnné, typu"
  E_SEMANTIC_REDEFINITION_ERROR = 4,

  // "Semantická chyba – nesprávný počet nebo typ argumentů při volání funkce,
  // nepovolené parametry funkce s anotací @main"
  E_SEMANTIC_ARGUMENT_ERROR = 5,

  // "Semantická chyba typové kompatibility nebo odvození typu ve výrazu,
  // přiřazení, inicializaci, návratové hodnotě či řídicí konstrukci"
  E_SEMANTIC_TYPE_ERROR = 6,

  // "Semantická chyba – pokus o zápis do nemodifikovatelné proměnné nebo
  // formálního parametru funkce"
  E_SEMANTIC_REWRITE_ERROR = 7,

  // "Ostatní sémantické chyby (např. nepovolený return, nevyčerpávající match
  // nebo nulový krok rozsahu)"
  E_SEMANTIC_OTHER_ERROR = 10,

  // "Interní chyba překladače neovlivněná vstupním programem (např. chyba
  // alokace paměti)"
  E_INTERNAL_ERROR = 99,
} ExitCode;

/**
 * @brief Flags for the IFJ26 compiler
 */
typedef struct {
  // Forces the nth malloc to fail (0 = ignore)
  int forceNthMallocFail;

  // Whether debug printing is enabled
  bool verbose;

  // Whether optimizations are disabled
  bool disableOptimizations;
} Flags;

/**
 * @brief Default flags used in the IFJ26 compiler
 */
#define DEFAULT_FLAGS                                                         \
  (Flags) {                                                                   \
    .forceNthMallocFail = 0, .verbose = false, .disableOptimizations = false, \
  }

/**
 * @brief Default flags used for unit and e2e tests
 */
#define DEFAULT_TEST_FLAGS                                                   \
  (Flags) {                                                                  \
    .forceNthMallocFail = 0, .verbose = true, .disableOptimizations = false, \
  }

/**
 * @brief Shared memory and error context for the IFJ26 compiler
 */
typedef struct Ctx {
  // Exit code of the program
  ExitCode exit_code;

  // Target for error jumps
  jmp_buf error_target;

  // Flags for the compiler
  Flags flags;

  // Abstract syntax tree of the program
  AstNode* ast;

  Parser* parser;

  // Global symbol table
  Symtable global_symtable;
  // Lexer state
  LexerCtx lexer;

  // Intermediate representation of the program
  IrProgram ir;
} Ctx;

/**
 * @brief Initializes the shared context using the provided flags
 *
 * Initializes the context with the given flags, sets default values but doesn't
 * allocate any memory do anything that could THROW as error context was not yet
 * initialized.
 *
 * @param[out] ctx Address of the context to initialize
 * @param[in] flags Flags for the compiler
 */
void ctx_init(Ctx* ctx, Flags flags);

/**
 * @brief Allocates memory for the shared context.
 *
 * @param[out] ctx Address of the context to allocate shared memory for
 */
void ctx_allocate(Ctx* ctx);

/**
 * @brief Parses supported command-line options into a Flags value.
 *
 * @param[in] argc Number of command-line arguments
 * @param[in] argv Command-line argument strings
 * @param[out] flags Parsed flags; only use them when the function returns true
 * @return Whether all arguments were supported and valid
 */
bool parse_cli_args(int argc, char* argv[], Flags* flags);

/**
 * @brief Frees context-owned memory and sets owned pointers to NULL.
 *
 * Preserves the error code. Repeated calls are safe after initialization.
 * Returns the exit code stored in the context.
 *
 * @param[in,out] ctx Non-null pointer to an initialized context.
 * @return The exit code stored in the context.
 */
ExitCode ctx_free(Ctx* ctx);

/**
 * @brief Allocates uninitialized memory and reports allocation failure.
 *
 * @param[in,out] ctx Non-null pointer to an initialized context.
 * @param[in] size Number of bytes to allocate
 * @param[in] file Non-null source file name of the allocation call.
 * @param[in] line Source line number of the allocation call.
 * @param[in] shouldJump Whether to jump to the error target on failure.
 *
 * @return Allocated memory, or NULL after setting E_INTERNAL_ERROR and printing
 * a diagnostic on failure (if shouldJump is false).
 */
void* ctx_malloc_helper(Ctx* ctx,
                        size_t size,
                        const char* file,
                        int line,
                        bool shouldJump);

/**
 * @brief Allocates memory and terminates on failure.
 *
 * On failure, sets E_INTERNAL_ERROR, prints a diagnostic and terminates the
 * compiler while correctly handling cleanup.
 *
 * @param[in] size Number of bytes to allocate
 *
 * @pre A variable named ctx points to an initialized Ctx.
 */
#define TRY_MALLOC(size) \
  ctx_malloc_helper(ctx, (size), __FILE__, __LINE__, true)

/**
 * @brief Allocates memory and terminates on failure after executing cleanup
 * code.
 *
 * On failure, sets E_INTERNAL_ERROR, prints a diagnostic, executes user
 * provided cleanup code, and terminates the compiler while correctly handling
 * cleanup.
 *
 * @param[in] size Number of bytes to allocate
 * @param[in] cleanup Code to execute on failure before terminating
 *
 * @pre A variable named ctx points to an initialized Ctx.
 * @example
 *  char* ptr1 = TRY_MALLOC(sizeof(char));
 *  char* ptr2 = TRY_MALLOC_OR(sizeof(char), {
 *    free(ptr1);
 *  });
 * @warning Does not behave as a single expression, for example
 *  if(condition)
 *    ptr = TRY_MALLOC_OR();
 *  else
 *    ptr = NULL;
 *  won't work as expected, needs to be written like this
 *  if(condition) {
 *    ptr = TRY_MALLOC_OR();
 *  } else
 *    ptr = NULL;
 */
#define TRY_MALLOC_OR(size, cleanup)                         \
  ctx_malloc_helper(ctx, (size), __FILE__, __LINE__, false); \
                                                             \
  /* If the allocation fails */                              \
  if (ctx->exit_code != E_OK) {                              \
    /* Run user provided cleanup code */                     \
    {cleanup}                                                \
                                                             \
    /* And jump to the error target */                       \
    longjmp(ctx->error_target, 1);                           \
  }

/**
 * @brief Helper function for reallocating memory
 *
 * If the reallocation fails, long jumps to the error target.
 *
 * @param[in,out] ctx Non-null pointer to an initialized context.
 * @param[in,out] ptr Pointer to the memory to reallocate.
 * @param[in] size Number of bytes to allocate
 * @param[in] file Non-null source file name of the allocation call.
 * @param[in] line Source line number of the allocation call.
 *
 * @pre The pointer must be non-null.
 * @pre The size must be positive.
 */
void* ctx_realloc_helper(Ctx* ctx,
                         void* ptr,
                         size_t size,
                         const char* file,
                         int line);

/**
 * @brief Reallocates memory and handles errors
 *
 * If the reallocation fails, the pointer stays unchanged and the compiler
 * execution is correctly terminated.
 *
 * @param[in,out] ptr Pointer to the memory to reallocate
 * @param[in] size Number of bytes to allocate
 *
 * @pre The pointer must be non-null.
 * @pre The size must be positive.
 * @pre A variable named ctx points to an initialized Ctx.
 */
#define TRY_REALLOC(ptr, size)                                          \
  do {                                                                  \
    if ((ptr) == NULL)                                                  \
      THROW(E_INTERNAL_ERROR, "Attempted realloc on NULL pointer");     \
                                                                        \
    (ptr) = ctx_realloc_helper(ctx, (ptr), (size), __FILE__, __LINE__); \
  } while (0)

/**
 * @brief Stores error code in ctx, prints diagnostic and source location to
 * stderr and jumps to the error target (if shouldJump is true).
 *
 * @param[in,out] ctx Non-null pointer to an initialized context.
 * @param[in] code Error code to report; must not be E_OK.
 * @param[in] file Non-null source file name of the error-reporting call.
 * @param[in] line Source line number of the error-reporting call.
 * @param[in] shouldJump If true, long jumps to the error target
 * @param[in] format Format string for the diagnostic message.
 * @param[in] ... Arguments for the format string.
 */
void throw_error_helper(Ctx* ctx,
                        ExitCode code,
                        const char* file,
                        int line,
                        bool shouldJump,
                        const char* format,
                        ...);

/**
 * @brief Reports an error and correctly terminates the compiler.
 *
 * @param[in] code Error code to report; must not be E_OK.
 * @param[in] ... Diagnostic message format string and arguments.
 *
 * @pre A variable named ctx holds a valid pointer to an initialized Ctx.
 * @warning Any required cleanup must occur before invoking this macro.
 */
#define THROW(code, ...) \
  throw_error_helper(ctx, code, __FILE__, __LINE__, true, __VA_ARGS__)

/**
 * @brief If the debug flag is set, prints a formatted diagnostic message.
 *
 * @param[in] ctx Non-null pointer to an initialized context.
 * @param[in] file Non-null source file name of the error-reporting call.
 * @param[in] line Source line number of the error-reporting call.
 * @param[in] format Format string for the diagnostic message.
 * @param[in] ... Arguments for the format string.
 */
void debug_print_helper(Ctx* ctx,
                        const char* file,
                        int line,
                        const char* format,
                        ...);

/**
 * @brief If the debug flag is set, prints a formatted diagnostic message.
 *
 * @param[in] ... Diagnostic message format string and arguments.
 *
 * @pre A variable named ctx holds a valid pointer to an initialized Ctx.
 */
#define DEBUG(...) debug_print_helper(ctx, __FILE__, __LINE__, __VA_ARGS__)

#endif
