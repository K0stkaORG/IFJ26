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
 * @brief Shared memory and error context for the IFJ26 compiler
 */
typedef struct {
  // Exit code of the program
  ExitCode exit_code;
} Ctx;

/**
 * @brief Initializes the shared context to default values
 *
 * @param[out] ctx Address of the context to initialize
 */
void ctx_init(Ctx* ctx);

/**
 * @brief Records an error and prints its diagnostic to stderr.
 *
 * Stores code if no error has been recorded. Also replaces
 * E_SEMANTIC_OTHER_ERROR with a specific semantic code (3–7).
 *
 * @param[in,out] ctx Non-null pointer to an initialized context.
 * @param[in] code Error code to report; must not be E_OK.
 * @param[in] format Format string for the diagnostic message.
 * @param[in] ... Arguments for the format string.
 */
void throw_error_helper(Ctx* ctx, ExitCode code, const char* format, ...);

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
#define THROW(code, ...)                        \
  do {                                          \
    throw_error_helper(ctx, code, __VA_ARGS__); \
                                                \
    return;                                     \
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
