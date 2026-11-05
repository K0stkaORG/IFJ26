/**
 * @file lexer.h
 * Project: IFJ26
 *
 * @author David Alois Bureš (xburesd00)
 * @date 13. 10. 2026
 *
 * @brief Header file for lexer.c
 *
 * This file defines the lexer token types and methods for lexer.c.
 */

// include guard
#ifndef IFJ26_LEXER_H
#define IFJ26_LEXER_H

/**
 * @brief Definition of lexical tokens
 */
typedef enum {
  IDENT,

  AT_MAIN,
  IMPORT,
  DEF,
  VAL,
  VAR,
  RETURN,
  IF,
  THEN,
  ELSE,
  WHILE,
  DO,

  TYPE_INT,
  TYPE_DOUBLE,
  TYPE_STRING,
  TYPE_UNIT,

  FOR,
  TYPE_BOOL,
  BOOL_TRUE,
  BOOL_FALSE,

  NUMBER_INT,
  NUMBER_DOUBLE,

  STRING,
  TRIPLE_STRING,
  S_STRING_START,
  S_STRING_END,
  TRIPLE_S_STRING_START,
  TRIPLE_S_STRING_END,

  OPERATOR_PLUS,
  OPERATOR_MINUS,
  OPERATOR_MULTIPLY,
  OPERATOR_DIVIDE,

  ASSIGN,
  COMPARISON_EQ,
  COMPARISON_NEQ,
  COMPARISON_G,
  COMPARISON_L,
  COMPARISON_GE,
  COMPARISON_LE,

  BOOL_AND,
  BOOL_OR,
  BOOL_NEG,

  L_PAREN,
  R_PAREN,
  COMMA,
  PERIOD,
  COLON,
  SEMICOLON,

  CONTROL_ARROW,

  INDENT,
  EOL_TOKEN,
  WHITESPACE,
  EOF_TOKEN
} TokenType;

/**
 * @brief Union defining the value of a token
 */
typedef union {
  int integer;
  double decimal;
  char* text;
} TokenValue;

/**
 * @brief structure defining the tokens
 */
typedef struct {
  TokenType type;
  TokenValue value;
  int line;
  int column;
} Token;

#endif /* IFJ26_LEXER_H */
