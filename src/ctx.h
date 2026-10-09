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

#endif
