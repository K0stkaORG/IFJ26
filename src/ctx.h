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

#include <stdbool.h>  // bool
#include <stddef.h>   // size_t

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
} Flags;

/**
 * @brief Shared memory and error context for the IFJ26 compiler
 */
typedef struct {
  // Exit code of the program
  ExitCode exit_code;

  // Flags for the compiler
  Flags flags;

  // Dummy temporary buffer
  char* buffer;
} Ctx;

/**
 * @brief Initializes the shared context to default values
 *
 * @param[out] ctx Address of the context to initialize
 */
void ctx_init(Ctx* ctx);

/**
 * @brief Frees context-owned memory and sets owned pointers to NULL.
 *
 * Preserves the error code. Repeated calls are safe after initialization.
 *
 * @param[in,out] ctx Non-null pointer to an initialized context.
 */
void ctx_free(Ctx* ctx);

/**
 * @brief Allocates uninitialized memory and reports allocation failure.
 *
 * @param[in,out] ctx Non-null pointer to an initialized context.
 * @param[in] size Number of bytes to allocate
 * @param[in] file Non-null source file name of the allocation call.
 * @param[in] line Source line number of the allocation call.
 *
 * @return Allocated memory, or NULL after setting E_INTERNAL_ERROR and printing
 * a diagnostic on failure.
 */
void* ctx_malloc_helper(Ctx* ctx, size_t size, const char* file, int line);

/**
 * @brief Allocates memory into and returns on failure.
 *
 * On failure, sets E_INTERNAL_ERROR, prints a diagnostic, and returns from the
 * calling function.
 *
 * @param[in] size Number of bytes to allocate
 *
 * @pre A variable named ctx points to an initialized Ctx.
 * @pre The calling function returns void.
 * @warning Does not behave as one expression. For example:
 *  if(condition)
 *    ptr = TRY_MALLOC()
 *  else
 *    ptr = TRY_MALLOC()
 *
 *  wont work as expected.
 */
#define TRY_MALLOC(size)                              \
  ctx_malloc_helper(ctx, (size), __FILE__, __LINE__); \
                                                      \
  if (ctx->exit_code != E_OK)                         \
    return;

/**
 * @brief Helper function for reallocating memory
 *
 * If the reallocation fails, the error code is stored in ctx->exit_code and the
 * original pointer is returned.
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
 * If the reallocation fails, the error code is stored in ctx->exit_code
 * and the calling function is terminated. Value of the pointer is not changed.
 *
 * @param[in,out] ptr Pointer to the memory to reallocate
 * @param[in] size Number of bytes to allocate
 *
 * @pre The pointer must be non-null.
 * @pre The size must be positive.
 * @pre A variable named ctx points to an initialized Ctx.
 * @pre The calling function returns void.
 */
#define TRY_REALLOC(ptr, size)                                          \
  do {                                                                  \
    if ((ptr) == NULL)                                                  \
      THROW(E_INTERNAL_ERROR, "Attempted realloc on NULL pointer");     \
                                                                        \
    (ptr) = ctx_realloc_helper(ctx, (ptr), (size), __FILE__, __LINE__); \
                                                                        \
    if (ctx->exit_code != E_OK)                                         \
      return;                                                           \
  } while (0);

/**
 * @brief Records an error and prints its diagnostic and source location to
 * stderr.
 *
 * Stores code if no error has been recorded. Also replaces
 * E_SEMANTIC_OTHER_ERROR with a specific semantic code (3–7).
 *
 * @param[in,out] ctx Non-null pointer to an initialized context.
 * @param[in] code Error code to report; must not be E_OK.
 * @param[in] file Non-null source file name of the error-reporting call.
 * @param[in] line Source line number of the error-reporting call.
 * @param[in] format Format string for the diagnostic message.
 * @param[in] ... Arguments for the format string.
 */
void throw_error_helper(Ctx* ctx,
                        ExitCode code,
                        const char* file,
                        int line,
                        const char* format,
                        ...);

/**
 * @brief Reports an error and returns from the calling function.
 *
 * Calls throw_error_helper() with the caller's ctx variable, then executes a
 * return without a value
 *
 * @param[in] code Error code to report; must not be E_OK.
 * @param[in] ... Diagnostic message format string and arguments.
 *
 * @pre A variable named ctx holds a valid pointer to an initialized Ctx.
 * @pre The calling function returns void.
 * @warning Any required cleanup must occur before invoking this macro.
 */
#define THROW(code, ...)                                            \
  do {                                                              \
    throw_error_helper(ctx, code, __FILE__, __LINE__, __VA_ARGS__); \
                                                                    \
    return;                                                         \
  } while (0)

/**
 * @brief Executes an expression and propagates any error code from ctx.
 *
 * If the expression fails (ctx->exit_code != E_OK), the error is propagated by
 * returning early from the calling function.
 *
 * @param[in] expr The expression to execute.
 *
 * @pre A variable named ctx holds a valid pointer to an initialized Ctx.
 * @pre The calling function returns void.
 * @warning Any required cleanup must occur before invoking this macro.
 * @note The expression executes even if ctx already contains an error. After
 * execution, the macro returns if ctx->exit_code is not E_OK.
 */
#define TRY(expr)                                      \
  do {                                                 \
    expr;                                              \
                                                       \
    /*If the expression fails, propagate the error. */ \
    if (ctx->exit_code != E_OK)                        \
      return;                                          \
  } while (0)

#endif
