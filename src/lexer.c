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
  START,                           // q0
  IDENTIFIER,                      // q1
  INTEGER,                         // q2
  WHITESPACE_STATE,                // q3
  QUOTE_START,                     // q4
  TEXT_IN_STRING_1,                // q5
  STRING_WITH_TEXT,                // q6
  EQUALS,                          // q7
  ASSIGNMENT,                      // q8
  LESS_THEN,                       // q9
  LESS_THEN_OR_EQUAL,              // q10
  GREATER_THEN,                    // q11
  GREATER_THEN_OR_EQUAL,           // q12
  NEGATION,                        // q13
  NOT_EQUAL,                       // q14
  LEFT_PARENTHESIS,                // q15
  UNIT,                            // q16
  RIGHT_PARENTHESIS,               // q17
  DOUBLE_AMPERSAND,                // q18
  AMPERSAND,                       // q19
  PIPE,                            // q20
  DOUBLE_PIPE,                     // q21
  PLUS,                            // q22
  MINUS,                           // q23
  ASTERISK,                        // q24
  SLASH,                           // q25
  EMPTY_STRING,                    // q26
  QUADRUPLE_QUOTE,                 // q27
  TRIPLE_QUOTE_TEXT_QUOTE,         // q28
  T_STRING_WITH_TEXT,              // q29
  TRIPLE_QUOTE_TEXT_DOUBLE_QUOTE,  // q30
  TRIPLE_STRING_STATE,             // q31
  NUMBER_DOT,                      // q32
  DOUBLE,                          // q33
  NUMBER_DOT_NUMBER_E,             // q34
  DOUBLE_WITH_EXPONENT,            // q35
  UNDERSCORE_1,                    // Q36
  UNDERSCORE_2,                    // Q37
  UNDERSCORE_3,                    // Q38
  DOUBLE_WITH_SIGNED_EXPONENT,     // q39
  PERIOD_STATE,                    // q40
  COMMENT,                         // q41
  COMMA_STATE,                     // q42
  COLON_STATE,                     // q43
  SEMICOLON_STATE,                 // q44
  AT,                              // q45
  LEFT_ARROW,                      // q46
  UNDERSCORE_4,                    // Q47
  EOL_OR_EOF,                      // q48
  AT_M,                            // q49
  AT_MA,                           // q50
  AT_MAI,                          // q51
  AT_MAIN_STATE,                   // q52
  ZERO,                            // q53
  ESCAPED_1,                       // Q54
  S,                               // q55
  DOUBLE_WITH_D_SUFFIX_1,          // Q56
  DOUBLE_WITH_D_SUFFIX_2,          // Q57
  SLASH_ASTERISK,                  // q58
  INSIDE_BLOCK_COMMENT,            // q59
  SLASH_DOUBLE_ASTERISK_1,         // Q60
  POTENTIAL_NESTING,               // q61       // FIX SPELLING
  BLOCK_COMMENT,                   // q62
  INSIDE_MULTILINE_BLOCK_COMMENT,  // q63
  SLASH_DOUBLE_ASTERISK_2,         // Q64
  MULTILINE_BLOCK_COMMENT,         // q65
  NESTING,                         // q66
  S_QUOTE,                         // q67
  S_DOUBLE_QUOTE,                  // q68
  S_TRIPLE_QUOTE,                  // q69
  DOLLAR_SIGN,                     // q70
  QUOTE_1,                         // q71
  QUOTE_2,                         // q72
  DOUBLE_QUOTE,                    // q73
  TRIPLE_QUOTE,                    // q74
  DOLLAR_EXPRESSION_START,         // q75
  DOLLAR_EXPRESSION_END,           // q76
  TEXT_IN_STRING_2,                // q77
  ESCAPED_2,                       // Q78
  DOUBLE_DOLLAR_SIGN,              // q79
  TRIPLE_S_STRING_STRING,          // q80
  UNIT_WITH_WHITESPACE,            // q81
} current_state;

// enum for internal fsm stack
enum {
  STACK_EMPTY,
  STACK_DOLLAR_EXPR,
  STACK_NESTED_COMMENT,
  STACK_S_STRING,
  STACK_MULTILINE_S_STRING,
};

typedef enum {
  TRANSITION_KEEP,
  TRANSITION_PUSH,
  TRANSITION_POP,
  TRANSITION_REPLACE,
} TransitionStackAction;

typedef struct {
  bool (*matches)(int character);  // Literal or character-class predicate.
  unsigned int stack_top_mask;     // Bit n permits stack symbol n.
  int next_state;                  // One of the existing state enum values.
  TransitionStackAction stack_action;
  int stack_symbol;  // Used only for push and replace.
} Transition;

static bool is_ascii(int character) {
  return character >= 0 && character <= 127;
}

static bool a3_digits_not_0(int character) {
  return character >= '1' && character <= '9';
}

static bool a4_digits(int character) {
  return character >= '0' && character <= '9';
}

static bool a16_letters(int character) {
  return (character >= 'A' && character <= 'Z') ||
         (character >= 'a' && character <= 'z');
}

static bool a2_ident_char(int character) {
  return a16_letters(character) || a4_digits(character) || character == '_';
}

static bool a5_whitespace(int character) {
  return character == ' ' || character == '\t' || character == '\n' ||
         character == '\r';
}

static bool a10_newline(int character) {
  return character == '\n' || character == '\r';
}

static bool a6_string_char(int character) {
  return is_ascii(character) && !a10_newline(character) && character != '"' &&
         character != '\\';
}

static bool a7_triple_string_char(int character) {
  return is_ascii(character) && character != '"';
}

static bool a8_exponent(int character) {
  return character == 'e' || character == 'E';
}

static bool a9_sign(int character) {
  return character == '+' || character == '-';
}

static bool a12_escape(int character) {
  return character == '\\' || character == '"' || character == 'n' ||
         character == 'r' || character == 't';
}

static bool a13_suffix(int character) {
  return character == 'd' || character == 'D';
}

static bool a14_comment_char(int character) {
  return is_ascii(character) && !a10_newline(character) && character != '*' &&
         character != '/';
}

static bool a15_multiline_comment_char(int character) {
  return is_ascii(character) && character != '*' && character != '/';
}

static bool a17_s_string_char(int character) {
  return a6_string_char(character) && character != '$';
}

static bool a18_triple_s_string_char(int character) {
  return a7_triple_string_char(character) && character != '$';
}

static bool a1_letter_not_s(int character) {
  return a16_letters(character) && character != 's';
}

static bool a11_line_comment(int character) {
  return is_ascii(character) && !a10_newline(character);
}

static const Transition identifier_transitions[] = {
    {
        .matches = a2_ident_char,
        .stack_top_mask = (1u << STACK_EMPTY) | (1u << STACK_DOLLAR_EXPR) |
                          (1u << STACK_NESTED_COMMENT) |
                          (1u << STACK_S_STRING) |
                          (1u << STACK_MULTILINE_S_STRING),
        .next_state = IDENTIFIER,
        .stack_action = TRANSITION_KEEP,
        .stack_symbol = STACK_EMPTY,  // Ignored when keeping the stack.
    },
};

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

    // fetching next char, from buffer if possible, otherwise save new to buffer
    if (buffer->count > Input.cursor) {
      current = buffer->data[Input.cursor];
      Input.cursor++;
    } else {
      current = getchar();
      if (current >= 0) {
        array_append(ctx, buffer, current);
      }
    }

    // top of the stack
    int top = stack.count == 0 ? STACK_EMPTY : stack.data[stack.count - 1];

    // tests for different transitions:
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
              case '}':
                if (top == STACK_DOLLAR_EXPR) {
                  array_remove(ctx, &stack, stack.count - 1);  // stack pop
                  next_state = DOLLAR_EXPRESSION_END;
                }
                break;
              default:
                break;
            }
          }
        }  //  if (top == STACK_EMPTY || top == STACK_DOLLAR_EXPR)
        else if (current == '$') {
          next_state = DOLLAR_SIGN;
        } else if (top == STACK_S_STRING) {
          if (current == '"') {
            array_remove(ctx, &stack, stack.count - 1);  // stack pop
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
