/**
 * @file lexer.c
 * Project: IFJ26
 *
 * @author David Alois Bureš (xburesd00)
 * @date 9. 11. 2026
 *
 * @brief Implementation of lexer
 *
 */

#include "lexer.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "array.h"
#include "ctx.h"

static struct {
  size_t line;
  size_t column;
  size_t cursor;

} Input;  // RENAME

static enum {
  START,
  IDENTIFIER,
  INTEGER,
  WHITESPACE_STATE,
  QUOTE_START,
  TEXT_IN_STRING_1,
  STRING_WITH_TEXT,
  EQUALS,
  ASSIGNMENT,
  LESS_THEN,
  LESS_THEN_OR_EQUAL,
  GREATER_THEN,
  GREATER_THEN_OR_EQUAL,
  NEGATION,
  NOT_EQUAL,
  LEFT_PARENTHESIS,
  UNIT,
  RIGHT_PARENTHESIS,
  DOUBLE_AMPERSAND,
  AMPERSAND,
  PIPE,
  DOUBLE_PIPE,
  PLUS,
  MINUS,
  ASTERISK,
  SLASH,
  EMPTY_STRING,
  QUADRUPLE_QUOTE,
  TRIPLE_QUOTE_TEXT_QUOTE,
  T_STRING_WITH_TEXT,
  TRIPLE_QUOTE_TEXT_DOUBLE_QUOTE,
  TRIPLE_STRING_STATE,
  NUMBER_DOT,
  DOUBLE,
  NUMBER_DOT_NUMBER_E,
  DOUBLE_WITH_EXPONENT,
  UNDERSCORE_1,  // Q36
  UNDERSCORE_2,  // Q37
  UNDERSCORE_3,  // Q38
  DOUBLE_WITH_SIGNED_EXPONENT,
  PERIOD_STATE,
  COMMENT,
  COMMA_STATE,
  COLON_STATE,
  SEMICOLON_STATE,
  AT,
  LEFT_ARROW,
  UNDERSCORE_4,  // Q47
  EOL_OR_EOF,
  AT_M,
  AT_MA,
  AT_MAI,
  AT_MAIN_STATE,
  ZERO,
  ESCAPED_1,  // Q54
  S,
  DOUBLE_WITH_D_SUFFIX_1,  // Q56
  DOUBLE_WITH_D_SUFFIX_2,  // Q57
  SLASH_ASTERISK,
  INSIDE_BLOCK_COMMENT,
  SLASH_DOUBLE_ASTERISK_1,  // Q60
  POTENTIAL_NESTING,        // FIX SPELLING
  BLOCK_COMMENT,
  INSIDE_MULTILINE_BLOCK_COMMENT,
  SLASH_DOUBLE_ASTERISK_2,  // Q64
  MULTILINE_BLOCK_COMMENT,
  NESTING,
  S_QUOTE,
  S_DOUBLE_QUOTE,
  S_TRIPLE_QUOTE,
  DOLLAR_SIGN,
  QUOTE_1,
  QUOTE_2,
  DOUBLE_QUOTE,
  TRIPLE_QUOTE,
  DOLLAR_EXPRESSION_START,
  DOLLAR_EXPRESSION_END,
  TEXT_IN_STRING_2,
  ESCAPED_2,  // Q78
  DOUBLE_DOLLAR_SIGN,
  TRIPLE_S_STRING_STRING,
  UNIT_WITH_WHITESPACE,
} current_state;

/**
 * @brief provides next token from the stdin stream
 *
 * @param ctx context of the compiler
 * @return next token from the stdin stream
 */
Token next_token(Ctx* ctx) {
  LexerByteBuffer* buffer = &ctx->lexer_buffer;
  Token token = {EOF_TOKEN, {0}, Input.line, Input.column};
  current_state = START;

  LexerByteBuffer stack = ARRAY_EMPTY;
  while (true) {
    int current;
    if (buffer->count > Input.cursor) {
      current = buffer->data[Input.cursor];
      Input.cursor++;
    } else {
      current = getchar();
      if (current >= 0) {
        array_append(ctx, buffer, current);
      }
    }

    enum {
      STACK_EMPTY,
      STACK_DOLLAR_EXPR,
      STACK_NESTED_COMMENT,
      STACK_S_STRING,
      STACK_MULTILINE_S_STRING,
    };

    int top = stack.count == 0 ? STACK_EMPTY : stack.data[stack.count - 1];
    bool ascii = current >= 0 && current <= 127;
    bool letter = (current >= 'A' && current <= 'Z') ||
                  (current >= 'a' && current <= 'z');          // A16
    bool digit = current >= '0' && current <= '9';             // A4
    bool identifier_char = letter || digit || current == '_';  // A2
    bool whitespace =
        (current >= '\t' && current <= '\r') || current == ' ';  // A5
    bool newline = current == '\r' || current == '\n';           // A10
    bool string_char =
        ascii && !newline && current != '"' && current != '\\';  // A6
    bool triple_char = ascii && current != '"';                  // A7
    bool exponent = current == 'e' || current == 'E';            // A8
    bool sign = current == '+' || current == '-';                // A9
    bool escape = current == '"' || current == '\\' || current == 'n' ||
                  current == 'r' || current == 't';  // A12
    bool suffix = current == 'd' || current == 'D';  // A13
    bool comment_char =
        ascii && !newline && current != '*' && current != '/';  // A14
    bool multiline_comment_char =
        ascii && current != '*' && current != '/';              // A15
    bool s_string_char = string_char && current != '$';         // A17
    bool triple_s_string_char = triple_char && current != '$';  // A18
    int next_state = -1;

    switch (current_state) {
      case START:
        if (top == STACK_EMPTY || top == STACK_DOLLAR_EXPR) {
          if (letter && current != 's') {  // A1
            next_state = IDENTIFIER;
          } else if (current >= '1' && current <= '9') {  // A3
            next_state = INTEGER;
          } else if (whitespace) {
            next_state = WHITESPACE_STATE;
          } else {
            switch (current) {
              case '"':
                next_state = QUOTE_START;
                break;
              case '=':
                next_state = EQUALS;
                break;
              case '<':
                next_state = LESS_THEN;
                break;
              case '>':
                next_state = GREATER_THEN;
                break;
              case '!':
                next_state = NEGATION;
                break;
              case '(':
                next_state = LEFT_PARENTHESIS;
                break;
              case ')':
                next_state = RIGHT_PARENTHESIS;
                break;
              case '&':
                next_state = AMPERSAND;
                break;
              case '|':
                next_state = PIPE;
                break;
              case '+':
                next_state = PLUS;
                break;
              case '-':
                next_state = MINUS;
                break;
              case '*':
                next_state = ASTERISK;
                break;
              case '/':
                next_state = SLASH;
                break;
              case '.':
                next_state = PERIOD_STATE;
                break;
              case ',':
                next_state = COMMA_STATE;
                break;
              case ':':
                next_state = COLON_STATE;
                break;
              case ';':
                next_state = SEMICOLON_STATE;
                break;
              case '_':
                next_state = UNDERSCORE_4;
                break;
              case '0':
                next_state = ZERO;
                break;
              case 's':
                next_state = S;
                break;
              case '@':
                if (top == STACK_EMPTY)
                  next_state = AT;
                break;
              case '$':
                if (top == STACK_DOLLAR_EXPR)
                  next_state = DOLLAR_SIGN;
                break;
              case '}':
                if (top == STACK_DOLLAR_EXPR) {
                  array_remove(ctx, &stack, stack.count - 1);
                  next_state = DOLLAR_EXPRESSION_END;
                }
                break;
              default:
                // FIX add default
            }
          }
        } else if (current == '$') {
          next_state = DOLLAR_SIGN;
        } else if (top == STACK_S_STRING) {
          if (current == '"') {
            array_remove(ctx, &stack, stack.count - 1);
            next_state = QUOTE_1;
          } else if (s_string_char) {
            next_state = TEXT_IN_STRING_2;
          } else if (current == '\\') {
            next_state = ESCAPED_2;
          }
        } else if (top == STACK_MULTILINE_S_STRING) {
          if (current == '"') {
            next_state = QUOTE_2;
          } else if (triple_s_string_char) {
            next_state = TRIPLE_S_STRING_STRING;
          }
        }
        break;

      case IDENTIFIER:
        if (identifier_char)
          next_state = IDENTIFIER;
        break;
      case INTEGER:
        if (digit)
          next_state = INTEGER;
        else if (current == '.')
          next_state = NUMBER_DOT;
        else if (current == '_')
          next_state = UNDERSCORE_1;
        else if (exponent)
          next_state = NUMBER_DOT_NUMBER_E;
        break;
      case WHITESPACE_STATE:
        if (whitespace)
          next_state = WHITESPACE_STATE;
        break;
      case QUOTE_START:
        if (string_char)
          next_state = TEXT_IN_STRING_1;
        else if (current == '"')
          next_state = EMPTY_STRING;
        else if (current == '\\')
          next_state = ESCAPED_1;
        break;
      case TEXT_IN_STRING_1:
        if (current == '"')
          next_state = STRING_WITH_TEXT;
        else if (string_char)
          next_state = TEXT_IN_STRING_1;
        else if (current == '\\')
          next_state = ESCAPED_1;
        break;
      case EQUALS:
        if (current == '=')
          next_state = ASSIGNMENT;
        break;
      case LESS_THEN:
        if (current == '=')
          next_state = LESS_THEN_OR_EQUAL;
        else if (current == '-')
          next_state = LEFT_ARROW;
        break;
      case GREATER_THEN:
        if (current == '=')
          next_state = GREATER_THEN_OR_EQUAL;
        break;
      case NEGATION:
        if (current == '=')
          next_state = NOT_EQUAL;
        break;
      case LEFT_PARENTHESIS:
        if (current == ')')
          next_state = UNIT;
        else if (whitespace)
          next_state = UNIT_WITH_WHITESPACE;
        break;
      case AMPERSAND:
        if (current == '&')
          next_state = DOUBLE_AMPERSAND;
        break;
      case PIPE:
        if (current == '|')
          next_state = DOUBLE_PIPE;
        break;
      case SLASH:
        if (current == '/')
          next_state = COMMENT;
        else if (current == '*')
          next_state = SLASH_ASTERISK;
        break;
      case EMPTY_STRING:
        if (current == '"')
          next_state = QUADRUPLE_QUOTE;
        break;
      case QUADRUPLE_QUOTE:
        if (current == '"')
          next_state = TRIPLE_QUOTE_TEXT_QUOTE;
        else if (triple_char)
          next_state = T_STRING_WITH_TEXT;
        break;
      case TRIPLE_QUOTE_TEXT_QUOTE:
        if (current == '"')
          next_state = TRIPLE_QUOTE_TEXT_DOUBLE_QUOTE;
        else if (triple_char)
          next_state = T_STRING_WITH_TEXT;
        break;
      case T_STRING_WITH_TEXT:
        if (current == '"')
          next_state = TRIPLE_QUOTE_TEXT_QUOTE;
        else if (triple_char)
          next_state = T_STRING_WITH_TEXT;
        break;
      case TRIPLE_QUOTE_TEXT_DOUBLE_QUOTE:
        if (current == '"')
          next_state = TRIPLE_STRING_STATE;
        else if (triple_char)
          next_state = T_STRING_WITH_TEXT;
        break;
      case NUMBER_DOT:
      case PERIOD_STATE:
        if (digit)
          next_state = DOUBLE;
        break;
      case DOUBLE:
        if (digit)
          next_state = DOUBLE;
        else if (exponent)
          next_state = NUMBER_DOT_NUMBER_E;
        else if (current == '_')
          next_state = UNDERSCORE_2;
        else if (suffix)
          next_state = DOUBLE_WITH_D_SUFFIX_1;
        break;
      case NUMBER_DOT_NUMBER_E:
        if (digit)
          next_state = DOUBLE_WITH_EXPONENT;
        else if (sign)
          next_state = DOUBLE_WITH_SIGNED_EXPONENT;
        break;
      case DOUBLE_WITH_EXPONENT:
        if (digit)
          next_state = DOUBLE_WITH_EXPONENT;
        else if (current == '_')
          next_state = UNDERSCORE_3;
        else if (suffix)
          next_state = DOUBLE_WITH_D_SUFFIX_2;
        break;
      case UNDERSCORE_1:
        if (digit)
          next_state = INTEGER;
        else if (current == '_')
          next_state = UNDERSCORE_1;
        break;
      case UNDERSCORE_2:
        if (digit)
          next_state = DOUBLE;
        else if (current == '_')
          next_state = UNDERSCORE_2;
        break;
      case UNDERSCORE_3:
        if (digit)
          next_state = DOUBLE_WITH_EXPONENT;
        else if (current == '_')
          next_state = UNDERSCORE_3;
        break;
      case DOUBLE_WITH_SIGNED_EXPONENT:
        if (digit)
          next_state = DOUBLE_WITH_EXPONENT;
        break;
      case COMMENT:
        if (newline)
          next_state = EOL_OR_EOF;
        else if (ascii)
          next_state = COMMENT;  // A11
        break;
      case AT:
        if (current == 'm')
          next_state = AT_M;
        break;
      case UNDERSCORE_4:
        if (identifier_char)
          next_state = IDENTIFIER;
        break;
      case AT_M:
        if (current == 'a')
          next_state = AT_MA;
        break;
      case AT_MA:
        if (current == 'i')
          next_state = AT_MAI;
        break;
      case AT_MAI:
        if (current == 'n')
          next_state = AT_MAIN_STATE;
        break;
      case ZERO:
        if (current == '.')
          next_state = NUMBER_DOT;
        break;
      case ESCAPED_1:
        if (escape)
          next_state = TEXT_IN_STRING_1;
        break;
      case S:
        if (identifier_char) {
          next_state = IDENTIFIER;
        } else if (current == '"') {
          array_append(ctx, &stack, STACK_S_STRING);
          next_state = S_QUOTE;
        }
        break;

      case SLASH_ASTERISK:
      case INSIDE_BLOCK_COMMENT:
        if (comment_char)
          next_state = INSIDE_BLOCK_COMMENT;
        else if (current == '*')
          next_state = SLASH_DOUBLE_ASTERISK_1;
        else if (current == '/')
          next_state = POTENTIAL_NESTING;
        else if (newline)
          next_state = INSIDE_MULTILINE_BLOCK_COMMENT;
        break;
      case SLASH_DOUBLE_ASTERISK_1:
        if (current == '/') {
          if (top == STACK_NESTED_COMMENT) {
            array_remove(ctx, &stack, stack.count - 1);
            next_state = INSIDE_BLOCK_COMMENT;
          } else {
            next_state = BLOCK_COMMENT;
          }
        } else if (comment_char)
          next_state = INSIDE_BLOCK_COMMENT;
        else if (current == '*')
          next_state = SLASH_DOUBLE_ASTERISK_1;
        else if (newline)
          next_state = INSIDE_MULTILINE_BLOCK_COMMENT;
        break;
      case POTENTIAL_NESTING:
        if (comment_char) {
          next_state = INSIDE_BLOCK_COMMENT;
        } else if (current == '*') {
          array_append(ctx, &stack, STACK_NESTED_COMMENT);
          next_state = INSIDE_BLOCK_COMMENT;
        } else if (current == '/')
          next_state = POTENTIAL_NESTING;
        else if (newline)
          next_state = INSIDE_MULTILINE_BLOCK_COMMENT;
        break;
      case INSIDE_MULTILINE_BLOCK_COMMENT:
        if (multiline_comment_char)
          next_state = INSIDE_MULTILINE_BLOCK_COMMENT;
        else if (current == '*')
          next_state = SLASH_DOUBLE_ASTERISK_2;
        else if (current == '/')
          next_state = NESTING;
        break;
      case SLASH_DOUBLE_ASTERISK_2:
        if (current == '/') {
          if (top == STACK_NESTED_COMMENT) {
            array_remove(ctx, &stack, stack.count - 1);
            next_state = INSIDE_MULTILINE_BLOCK_COMMENT;
          } else {
            next_state = MULTILINE_BLOCK_COMMENT;
          }
        } else if (current == '*')
          next_state = SLASH_DOUBLE_ASTERISK_2;
        else if (multiline_comment_char)
          next_state = INSIDE_MULTILINE_BLOCK_COMMENT;
        break;
      case NESTING:
        if (current == '/') {
          next_state = NESTING;
        } else if (current == '*') {
          array_append(ctx, &stack, STACK_NESTED_COMMENT);
          next_state = INSIDE_MULTILINE_BLOCK_COMMENT;
        } else if (multiline_comment_char) {
          next_state = INSIDE_MULTILINE_BLOCK_COMMENT;
        }
        break;

      case S_QUOTE:
        if (current == '"')
          next_state = S_DOUBLE_QUOTE;
        break;
      case S_DOUBLE_QUOTE:
        if (current == '"' && top != STACK_EMPTY) {
          stack.data[stack.count - 1] = STACK_MULTILINE_S_STRING;
          next_state = S_TRIPLE_QUOTE;
        }
        break;
      case DOLLAR_SIGN:
        if (letter) {
          next_state = IDENTIFIER;
        } else if (current == '{') {
          array_append(ctx, &stack, STACK_DOLLAR_EXPR);
          next_state = DOLLAR_EXPRESSION_START;
        } else if (current == '_')
          next_state = UNDERSCORE_4;
        else if (current == '$')
          next_state = DOUBLE_DOLLAR_SIGN;
        break;
      case QUOTE_2:
        if (current == '"')
          next_state = DOUBLE_QUOTE;
        else if (triple_s_string_char)
          next_state = TRIPLE_S_STRING_STRING;
        break;
      case DOUBLE_QUOTE:
        if (current == '"' && top != STACK_EMPTY) {
          array_remove(ctx, &stack, stack.count - 1);
          next_state = TRIPLE_QUOTE;
        } else if (triple_s_string_char) {
          next_state = TRIPLE_S_STRING_STRING;
        }
        break;
      case TEXT_IN_STRING_2:
        if (s_string_char)
          next_state = TEXT_IN_STRING_2;
        else if (current == '\\')
          next_state = ESCAPED_2;
        break;
      case ESCAPED_2:
        if (escape)
          next_state = TEXT_IN_STRING_2;
        break;
      case TRIPLE_S_STRING_STRING:
        if (triple_s_string_char)
          next_state = TRIPLE_S_STRING_STRING;
        break;
      case UNIT_WITH_WHITESPACE:
        if (current == ')')
          next_state = UNIT;
        else if (whitespace)
          next_state = UNIT_WITH_WHITESPACE;
        break;

      // These states have no outgoing transitions in R.
      case STRING_WITH_TEXT:
      case ASSIGNMENT:
      case LESS_THEN_OR_EQUAL:
      case GREATER_THEN_OR_EQUAL:
      case NOT_EQUAL:
      case UNIT:
      case RIGHT_PARENTHESIS:
      case DOUBLE_AMPERSAND:
      case DOUBLE_PIPE:
      case PLUS:
      case MINUS:
      case ASTERISK:
      case TRIPLE_STRING_STATE:
      case COMMA_STATE:
      case COLON_STATE:
      case SEMICOLON_STATE:
      case LEFT_ARROW:
      case EOL_OR_EOF:
      case AT_MAIN_STATE:
      case DOUBLE_WITH_D_SUFFIX_1:
      case DOUBLE_WITH_D_SUFFIX_2:
      case BLOCK_COMMENT:
      case MULTILINE_BLOCK_COMMENT:
      case S_TRIPLE_QUOTE:
      case QUOTE_1:
      case TRIPLE_QUOTE:
      case DOLLAR_EXPRESSION_START:
      case DOLLAR_EXPRESSION_END:
      case DOUBLE_DOLLAR_SIGN:
        break;
    }
    // Token finalization and lookahead handling remain to be implemented.
    if (next_state < 0)
      break;
    current_state = next_state;
  }

  array_free(&stack);
  return token;
}
