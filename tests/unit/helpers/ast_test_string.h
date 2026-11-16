/**
 * @file ast_test_string.h
 * Project: IFJ26
 *
 * @author Lukáš Pollák (xpollal00)
 * @date 15. 11. 2026
 *
 * @brief Test-only printer rendering an AST as a compact S-expression
 *
 * Format: `(KIND name-or-value [FLAGS] child...)`. A NONE placeholder is
 * printed bare as `NONE`. Type annotations print their type
 * (`(TYPE_ANNOTATION Int)`), string literals are quoted with C escapes.
 *
 * Semantic annotations are printed only when set, after the flags and before
 * the children, so an AST the parser produces prints as before:
 *   ` :Int`     the node's data_type (not for a type annotation, which already
 *               prints its type)
 *   ` @3`       a variable symbol's slot (ir_index)
 *   ` fn2`      a user function symbol's function id (ir_index)
 *   ` builtin`  a function symbol on a node flagged BUILTIN
 * The BUILTIN flag is listed with the others (`[BUILTIN]`), INT_TO_DOUBLE is
 * `(INT_TO_DOUBLE :Double expr)`. The DEF checking flags are never printed.
 *
 * Example: `(PROGRAM (DEF [MAIN] (IDENT main) NONE (BLOCK (UNIT_LIT))
 * (PARAM_LIST)))`
 */

#ifndef TESTS_AST_TEST_STRING_H
#define TESTS_AST_TEST_STRING_H

#include "ast.h"  // AstNode

/**
 * @brief Renders node and its subtree as an S-expression.
 *
 * @param[in] node Node to render, NULL renders as `NULL`
 * @return Heap string the caller frees with free()
 */
char* ast_to_test_string(const AstNode* node);

#endif
