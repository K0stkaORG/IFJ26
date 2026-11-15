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
