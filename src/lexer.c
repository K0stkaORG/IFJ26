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
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "array.h"
#include "ctx.h"

typedef enum {
  START,                           // q0
  IDENTIFIER,                      // q1
  INTEGER_STATE,                   // q2
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
  POTENTIAL_NESTING,               // q61
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
  DOLLAR_EXPRESSION_START_STATE,   // q75
  DOLLAR_EXPRESSION_END_STATE,     // q76
  TEXT_IN_STRING_2,                // q77
  ESCAPED_2,                       // Q78
  DOUBLE_DOLLAR_SIGN,              // q79
  TRIPLE_S_STRING_STRING,          // q80
  UNIT_WITH_WHITESPACE,            // q81
} State;

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

// AI generated by: Zed / GPT-6.1 Sol
// Prompt: "Please add the remaining transitions from
// [@lexer.txt](file:///home/daalbu/Code/IFJ26/lexer.txt)" Changes: match the
// complete A5 whitespace class used by the supplied transitions.
static bool a5_whitespace(int character) {
  return character == ' ' || (character >= '\t' && character <= '\r');
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

// BEGIN GENERATED LEXER TRANSITIONS
// AI generated by: Zed / GPT-6.1 Sol
// Prompt: "Please add the remaining transitions from
// [@lexer.txt](file:///home/daalbu/Code/IFJ26/lexer.txt)"Changes: compact
// transcription of lexer.txt; regenerate with
// scripts/generate_lexer_transitions.py.

// AI generated by: Zed / GPT-6.1 Sol
// Prompt: "There is an updated
// [@lexer.txt](file:///home/daalbu/Code/IFJ26/lexer.txt) Can you implement the
// change and also make sure that DOLLAR_EXPRESSION_START and
// DOLLAR_EXPRESSION_END tokens are properly emmited?" Changes: Regenerate the
// supplied automaton with non-conflicting interpolation state names.
typedef struct {
  bool accepting;
  const Transition* transitions;
  size_t count;
} StateTransitions;

// Stack masks and transition initializers.
#define TOP(symbol) (1u << (symbol))
#define ALL_STACK_TOPS ((1u << (STACK_MULTILINE_S_STRING + 1)) - 1u)
#define CODE_TOPS (TOP(STACK_EMPTY) | TOP(STACK_DOLLAR_EXPR))

#define KEEP(matcher, tops, next)   \
  {.matches = (matcher),            \
   .stack_top_mask = (tops),        \
   .next_state = (next),            \
   .stack_action = TRANSITION_KEEP, \
   .stack_symbol = STACK_EMPTY}
#define PUSH(matcher, tops, next, symbol) \
  {.matches = (matcher),                  \
   .stack_top_mask = (tops),              \
   .next_state = (next),                  \
   .stack_action = TRANSITION_PUSH,       \
   .stack_symbol = (symbol)}
#define POP(matcher, tops, next)   \
  {.matches = (matcher),           \
   .stack_top_mask = (tops),       \
   .next_state = (next),           \
   .stack_action = TRANSITION_POP, \
   .stack_symbol = STACK_EMPTY}
#define REPLACE(matcher, tops, next, symbol) \
  {.matches = (matcher),                     \
   .stack_top_mask = (tops),                 \
   .next_state = (next),                     \
   .stack_action = TRANSITION_REPLACE,       \
   .stack_symbol = (symbol)}
#define OUTGOING(array, is_accepting) \
  {.accepting = (is_accepting),       \
   .transitions = (array),            \
   .count = sizeof(array) / sizeof((array)[0])}

// Literal matchers.
static bool literal_negation(int character) {
  return character == '!';
}

static bool literal_quote(int character) {
  return character == '"';
}

static bool literal_dollar(int character) {
  return character == '$';
}

static bool literal_ampersand(int character) {
  return character == '&';
}

static bool literal_left_parenthesis(int character) {
  return character == '(';
}

static bool literal_right_parenthesis(int character) {
  return character == ')';
}

static bool literal_asterisk(int character) {
  return character == '*';
}

static bool literal_plus(int character) {
  return character == '+';
}

static bool literal_comma(int character) {
  return character == ',';
}

static bool literal_minus(int character) {
  return character == '-';
}

static bool literal_period(int character) {
  return character == '.';
}

static bool literal_slash(int character) {
  return character == '/';
}

static bool literal_zero(int character) {
  return character == '0';
}

static bool literal_colon(int character) {
  return character == ':';
}

static bool literal_semicolon(int character) {
  return character == ';';
}

static bool literal_less_than(int character) {
  return character == '<';
}

static bool literal_equals(int character) {
  return character == '=';
}

static bool literal_greater_than(int character) {
  return character == '>';
}

static bool literal_at(int character) {
  return character == '@';
}

static bool literal_backslash(int character) {
  return character == '\\';
}

static bool literal_underscore(int character) {
  return character == '_';
}

static bool literal_a(int character) {
  return character == 'a';
}

static bool literal_i(int character) {
  return character == 'i';
}

static bool literal_m(int character) {
  return character == 'm';
}

static bool literal_n(int character) {
  return character == 'n';
}

static bool literal_s(int character) {
  return character == 's';
}

static bool literal_left_brace(int character) {
  return character == '{';
}

static bool literal_pipe(int character) {
  return character == '|';
}

static bool literal_right_brace(int character) {
  return character == '}';
}

// Outgoing transitions, in the q0..q81 order of lexer.txt.
static const Transition start_transitions[] = {
    KEEP(a1_letter_not_s, CODE_TOPS, IDENTIFIER),
    KEEP(a3_digits_not_0, CODE_TOPS, INTEGER_STATE),
    KEEP(a5_whitespace, CODE_TOPS, WHITESPACE_STATE),
    KEEP(literal_quote, CODE_TOPS, QUOTE_START),
    KEEP(literal_equals, CODE_TOPS, EQUALS),
    KEEP(literal_less_than, CODE_TOPS, LESS_THEN),
    KEEP(literal_greater_than, CODE_TOPS, GREATER_THEN),
    KEEP(literal_negation, CODE_TOPS, NEGATION),
    KEEP(literal_left_parenthesis, CODE_TOPS, LEFT_PARENTHESIS),
    KEEP(literal_right_parenthesis, CODE_TOPS, RIGHT_PARENTHESIS),
    KEEP(literal_ampersand, CODE_TOPS, AMPERSAND),
    KEEP(literal_pipe, CODE_TOPS, PIPE),
    KEEP(literal_plus, CODE_TOPS, PLUS),
    KEEP(literal_minus, CODE_TOPS, MINUS),
    KEEP(literal_asterisk, CODE_TOPS, ASTERISK),
    KEEP(literal_slash, CODE_TOPS, SLASH),
    KEEP(literal_period, CODE_TOPS, PERIOD_STATE),
    KEEP(literal_comma, CODE_TOPS, COMMA_STATE),
    KEEP(literal_colon, CODE_TOPS, COLON_STATE),
    KEEP(literal_semicolon, CODE_TOPS, SEMICOLON_STATE),
    KEEP(literal_at, TOP(STACK_EMPTY), AT),
    KEEP(literal_underscore, CODE_TOPS, UNDERSCORE_4),
    KEEP(literal_zero, CODE_TOPS, ZERO),
    KEEP(literal_s, CODE_TOPS, S),
    KEEP(literal_dollar,
         TOP(STACK_S_STRING) | TOP(STACK_MULTILINE_S_STRING),
         DOLLAR_SIGN),
    POP(literal_quote, TOP(STACK_S_STRING), QUOTE_1),
    KEEP(literal_quote, TOP(STACK_MULTILINE_S_STRING), QUOTE_2),
    POP(literal_right_brace,
        TOP(STACK_DOLLAR_EXPR),
        DOLLAR_EXPRESSION_END_STATE),
    KEEP(a17_s_string_char, TOP(STACK_S_STRING), TEXT_IN_STRING_2),
    KEEP(literal_backslash, TOP(STACK_S_STRING), ESCAPED_2),
    KEEP(a18_triple_s_string_char,
         TOP(STACK_MULTILINE_S_STRING),
         TRIPLE_S_STRING_STRING),
};  // q0

static const Transition identifier_transitions[] = {
    KEEP(a2_ident_char, ALL_STACK_TOPS, IDENTIFIER),
};  // q1

static const Transition integer_state_transitions[] = {
    KEEP(a4_digits, ALL_STACK_TOPS, INTEGER_STATE),
    KEEP(literal_period, ALL_STACK_TOPS, NUMBER_DOT),
    KEEP(literal_underscore, ALL_STACK_TOPS, UNDERSCORE_1),
    KEEP(a8_exponent, ALL_STACK_TOPS, NUMBER_DOT_NUMBER_E),
};  // q2

static const Transition whitespace_state_transitions[] = {
    KEEP(a5_whitespace, ALL_STACK_TOPS, WHITESPACE_STATE),
};  // q3

static const Transition quote_start_transitions[] = {
    KEEP(a6_string_char, ALL_STACK_TOPS, TEXT_IN_STRING_1),
    KEEP(literal_quote, ALL_STACK_TOPS, EMPTY_STRING),
    KEEP(literal_backslash, ALL_STACK_TOPS, ESCAPED_1),
};  // q4

static const Transition text_in_string_1_transitions[] = {
    KEEP(literal_quote, ALL_STACK_TOPS, STRING_WITH_TEXT),
    KEEP(a6_string_char, ALL_STACK_TOPS, TEXT_IN_STRING_1),
    KEEP(literal_backslash, ALL_STACK_TOPS, ESCAPED_1),
};  // q5

static const Transition equals_transitions[] = {
    KEEP(literal_equals, ALL_STACK_TOPS, ASSIGNMENT),
};  // q7

static const Transition less_then_transitions[] = {
    KEEP(literal_equals, ALL_STACK_TOPS, LESS_THEN_OR_EQUAL),
    KEEP(literal_minus, ALL_STACK_TOPS, LEFT_ARROW),
};  // q9

static const Transition greater_then_transitions[] = {
    KEEP(literal_equals, ALL_STACK_TOPS, GREATER_THEN_OR_EQUAL),
};  // q11

static const Transition negation_transitions[] = {
    KEEP(literal_equals, ALL_STACK_TOPS, NOT_EQUAL),
};  // q13

static const Transition left_parenthesis_transitions[] = {
    KEEP(literal_right_parenthesis, ALL_STACK_TOPS, UNIT),
    KEEP(a5_whitespace, ALL_STACK_TOPS, UNIT_WITH_WHITESPACE),
};  // q15

static const Transition ampersand_transitions[] = {
    KEEP(literal_ampersand, ALL_STACK_TOPS, DOUBLE_AMPERSAND),
};  // q19

static const Transition pipe_transitions[] = {
    KEEP(literal_pipe, ALL_STACK_TOPS, DOUBLE_PIPE),
};  // q20

static const Transition slash_transitions[] = {
    KEEP(literal_slash, ALL_STACK_TOPS, COMMENT),
    KEEP(literal_asterisk, ALL_STACK_TOPS, SLASH_ASTERISK),
};  // q25

static const Transition empty_string_transitions[] = {
    KEEP(literal_quote, ALL_STACK_TOPS, QUADRUPLE_QUOTE),
};  // q26

static const Transition quadruple_quote_transitions[] = {
    KEEP(literal_quote, ALL_STACK_TOPS, TRIPLE_QUOTE_TEXT_QUOTE),
    KEEP(a7_triple_string_char, ALL_STACK_TOPS, T_STRING_WITH_TEXT),
};  // q27

static const Transition triple_quote_text_quote_transitions[] = {
    KEEP(literal_quote, ALL_STACK_TOPS, TRIPLE_QUOTE_TEXT_DOUBLE_QUOTE),
    KEEP(a7_triple_string_char, ALL_STACK_TOPS, T_STRING_WITH_TEXT),
};  // q28

static const Transition t_string_with_text_transitions[] = {
    KEEP(literal_quote, ALL_STACK_TOPS, TRIPLE_QUOTE_TEXT_QUOTE),
    KEEP(a7_triple_string_char, ALL_STACK_TOPS, T_STRING_WITH_TEXT),
};  // q29

static const Transition triple_quote_text_double_quote_transitions[] = {
    KEEP(literal_quote, ALL_STACK_TOPS, TRIPLE_STRING_STATE),
    KEEP(a7_triple_string_char, ALL_STACK_TOPS, T_STRING_WITH_TEXT),
};  // q30

static const Transition number_dot_transitions[] = {
    KEEP(a4_digits, ALL_STACK_TOPS, DOUBLE),
};  // q32

static const Transition double_transitions[] = {
    KEEP(a4_digits, ALL_STACK_TOPS, DOUBLE),
    KEEP(a8_exponent, ALL_STACK_TOPS, NUMBER_DOT_NUMBER_E),
    KEEP(literal_underscore, ALL_STACK_TOPS, UNDERSCORE_2),
    KEEP(a13_suffix, ALL_STACK_TOPS, DOUBLE_WITH_D_SUFFIX_1),
};  // q33

static const Transition number_dot_number_e_transitions[] = {
    KEEP(a4_digits, ALL_STACK_TOPS, DOUBLE_WITH_EXPONENT),
    KEEP(a9_sign, ALL_STACK_TOPS, DOUBLE_WITH_SIGNED_EXPONENT),
};  // q34

static const Transition double_with_exponent_transitions[] = {
    KEEP(a4_digits, ALL_STACK_TOPS, DOUBLE_WITH_EXPONENT),
    KEEP(literal_underscore, ALL_STACK_TOPS, UNDERSCORE_3),
    KEEP(a13_suffix, ALL_STACK_TOPS, DOUBLE_WITH_D_SUFFIX_2),
};  // q35

static const Transition underscore_1_transitions[] = {
    KEEP(a4_digits, ALL_STACK_TOPS, INTEGER_STATE),
    KEEP(literal_underscore, ALL_STACK_TOPS, UNDERSCORE_1),
};  // q36

static const Transition underscore_2_transitions[] = {
    KEEP(a4_digits, ALL_STACK_TOPS, DOUBLE),
    KEEP(literal_underscore, ALL_STACK_TOPS, UNDERSCORE_2),
};  // q37

static const Transition underscore_3_transitions[] = {
    KEEP(a4_digits, ALL_STACK_TOPS, DOUBLE_WITH_EXPONENT),
    KEEP(literal_underscore, ALL_STACK_TOPS, UNDERSCORE_3),
};  // q38

static const Transition double_with_signed_exponent_transitions[] = {
    KEEP(a4_digits, ALL_STACK_TOPS, DOUBLE_WITH_EXPONENT),
};  // q39

static const Transition period_state_transitions[] = {
    KEEP(a4_digits, ALL_STACK_TOPS, DOUBLE),
};  // q40

static const Transition comment_transitions[] = {
    KEEP(a10_newline, ALL_STACK_TOPS, EOL_OR_EOF),
    KEEP(a11_line_comment, ALL_STACK_TOPS, COMMENT),
};  // q41

static const Transition at_transitions[] = {
    KEEP(literal_m, ALL_STACK_TOPS, AT_M),
};  // q45

static const Transition underscore_4_transitions[] = {
    KEEP(a2_ident_char, ALL_STACK_TOPS, IDENTIFIER),
};  // q47

static const Transition at_m_transitions[] = {
    KEEP(literal_a, ALL_STACK_TOPS, AT_MA),
};  // q49

static const Transition at_ma_transitions[] = {
    KEEP(literal_i, ALL_STACK_TOPS, AT_MAI),
};  // q50

static const Transition at_mai_transitions[] = {
    KEEP(literal_n, ALL_STACK_TOPS, AT_MAIN_STATE),
};  // q51

static const Transition zero_transitions[] = {
    KEEP(literal_period, ALL_STACK_TOPS, NUMBER_DOT),
};  // q53

static const Transition escaped_1_transitions[] = {
    KEEP(a12_escape, ALL_STACK_TOPS, TEXT_IN_STRING_1),
};  // q54

static const Transition s_transitions[] = {
    KEEP(a2_ident_char, ALL_STACK_TOPS, IDENTIFIER),
    PUSH(literal_quote, ALL_STACK_TOPS, S_QUOTE, STACK_S_STRING),
};  // q55

static const Transition slash_asterisk_transitions[] = {
    KEEP(a14_comment_char, ALL_STACK_TOPS, INSIDE_BLOCK_COMMENT),
    KEEP(literal_asterisk, ALL_STACK_TOPS, SLASH_DOUBLE_ASTERISK_1),
    KEEP(literal_slash, ALL_STACK_TOPS, POTENTIAL_NESTING),
    KEEP(a10_newline, ALL_STACK_TOPS, INSIDE_MULTILINE_BLOCK_COMMENT),
};  // q58

static const Transition inside_block_comment_transitions[] = {
    KEEP(literal_asterisk, ALL_STACK_TOPS, SLASH_DOUBLE_ASTERISK_1),
    KEEP(literal_slash, ALL_STACK_TOPS, POTENTIAL_NESTING),
    KEEP(a14_comment_char, ALL_STACK_TOPS, INSIDE_BLOCK_COMMENT),
    KEEP(a10_newline, ALL_STACK_TOPS, INSIDE_MULTILINE_BLOCK_COMMENT),
};  // q59

static const Transition slash_double_asterisk_1_transitions[] = {
    KEEP(literal_slash,
         ALL_STACK_TOPS & ~TOP(STACK_NESTED_COMMENT),
         BLOCK_COMMENT),
    KEEP(a14_comment_char, ALL_STACK_TOPS, INSIDE_BLOCK_COMMENT),
    POP(literal_slash, TOP(STACK_NESTED_COMMENT), INSIDE_BLOCK_COMMENT),
    KEEP(literal_asterisk, ALL_STACK_TOPS, SLASH_DOUBLE_ASTERISK_1),
    KEEP(a10_newline, ALL_STACK_TOPS, INSIDE_MULTILINE_BLOCK_COMMENT),
};  // q60

static const Transition potential_nesting_transitions[] = {
    KEEP(a14_comment_char, ALL_STACK_TOPS, INSIDE_BLOCK_COMMENT),
    PUSH(literal_asterisk,
         ALL_STACK_TOPS,
         INSIDE_BLOCK_COMMENT,
         STACK_NESTED_COMMENT),
    KEEP(a10_newline, ALL_STACK_TOPS, INSIDE_MULTILINE_BLOCK_COMMENT),
    KEEP(literal_slash, ALL_STACK_TOPS, POTENTIAL_NESTING),
};  // q61

static const Transition inside_multiline_block_comment_transitions[] = {
    KEEP(a15_multiline_comment_char,
         ALL_STACK_TOPS,
         INSIDE_MULTILINE_BLOCK_COMMENT),
    KEEP(literal_asterisk, ALL_STACK_TOPS, SLASH_DOUBLE_ASTERISK_2),
    KEEP(literal_slash, ALL_STACK_TOPS, NESTING),
};  // q63

static const Transition slash_double_asterisk_2_transitions[] = {
    KEEP(literal_slash,
         ALL_STACK_TOPS & ~TOP(STACK_NESTED_COMMENT),
         MULTILINE_BLOCK_COMMENT),
    KEEP(literal_asterisk, ALL_STACK_TOPS, SLASH_DOUBLE_ASTERISK_2),
    KEEP(a15_multiline_comment_char,
         ALL_STACK_TOPS,
         INSIDE_MULTILINE_BLOCK_COMMENT),
    POP(literal_slash,
        TOP(STACK_NESTED_COMMENT),
        INSIDE_MULTILINE_BLOCK_COMMENT),
};  // q64

static const Transition nesting_transitions[] = {
    KEEP(literal_slash, ALL_STACK_TOPS, NESTING),
    PUSH(literal_asterisk,
         ALL_STACK_TOPS,
         INSIDE_MULTILINE_BLOCK_COMMENT,
         STACK_NESTED_COMMENT),
    KEEP(a15_multiline_comment_char,
         ALL_STACK_TOPS,
         INSIDE_MULTILINE_BLOCK_COMMENT),
};  // q66

static const Transition s_quote_transitions[] = {
    KEEP(literal_quote, ALL_STACK_TOPS, S_DOUBLE_QUOTE),
};  // q67

static const Transition s_double_quote_transitions[] = {
    REPLACE(literal_quote,
            TOP(STACK_DOLLAR_EXPR) | TOP(STACK_NESTED_COMMENT) |
                TOP(STACK_S_STRING),
            S_TRIPLE_QUOTE,
            STACK_MULTILINE_S_STRING),
    KEEP(literal_quote, TOP(STACK_MULTILINE_S_STRING), S_TRIPLE_QUOTE),
};  // q68

static const Transition dollar_sign_transitions[] = {
    KEEP(a16_letters, ALL_STACK_TOPS, IDENTIFIER),
    PUSH(literal_left_brace,
         ALL_STACK_TOPS,
         DOLLAR_EXPRESSION_START_STATE,
         STACK_DOLLAR_EXPR),
    KEEP(literal_underscore, ALL_STACK_TOPS, UNDERSCORE_4),
    KEEP(literal_dollar, ALL_STACK_TOPS, DOUBLE_DOLLAR_SIGN),
};  // q70

static const Transition quote_2_transitions[] = {
    KEEP(literal_quote, ALL_STACK_TOPS, DOUBLE_QUOTE),
    KEEP(a18_triple_s_string_char, ALL_STACK_TOPS, TRIPLE_S_STRING_STRING),
};  // q72

static const Transition double_quote_transitions[] = {
    POP(literal_quote, ALL_STACK_TOPS & ~TOP(STACK_EMPTY), TRIPLE_QUOTE),
    KEEP(a18_triple_s_string_char, ALL_STACK_TOPS, TRIPLE_S_STRING_STRING),
};  // q73

static const Transition text_in_string_2_transitions[] = {
    KEEP(a17_s_string_char, ALL_STACK_TOPS, TEXT_IN_STRING_2),
    KEEP(literal_backslash, ALL_STACK_TOPS, ESCAPED_2),
};  // q77

static const Transition escaped_2_transitions[] = {
    KEEP(a12_escape, ALL_STACK_TOPS, TEXT_IN_STRING_2),
};  // q78

static const Transition triple_s_string_string_transitions[] = {
    KEEP(a18_triple_s_string_char, ALL_STACK_TOPS, TRIPLE_S_STRING_STRING),
};  // q80

static const Transition unit_with_whitespace_transitions[] = {
    KEEP(literal_right_parenthesis, ALL_STACK_TOPS, UNIT),
    KEEP(a5_whitespace, ALL_STACK_TOPS, UNIT_WITH_WHITESPACE),
};  // q81

// States without outgoing rules have a NULL pointer and zero count.
static const StateTransitions state_transitions[UNIT_WITH_WHITESPACE + 1] = {
    [START] = OUTGOING(start_transitions, false),
    [IDENTIFIER] = OUTGOING(identifier_transitions, true),
    [INTEGER_STATE] = OUTGOING(integer_state_transitions, true),
    [WHITESPACE_STATE] = OUTGOING(whitespace_state_transitions, true),
    [QUOTE_START] = OUTGOING(quote_start_transitions, false),
    [TEXT_IN_STRING_1] = OUTGOING(text_in_string_1_transitions, false),
    [STRING_WITH_TEXT] = {.accepting = true},
    [EQUALS] = OUTGOING(equals_transitions, true),
    [ASSIGNMENT] = {.accepting = true},
    [LESS_THEN] = OUTGOING(less_then_transitions, true),
    [LESS_THEN_OR_EQUAL] = {.accepting = true},
    [GREATER_THEN] = OUTGOING(greater_then_transitions, true),
    [GREATER_THEN_OR_EQUAL] = {.accepting = true},
    [NEGATION] = OUTGOING(negation_transitions, true),
    [NOT_EQUAL] = {.accepting = true},
    [LEFT_PARENTHESIS] = OUTGOING(left_parenthesis_transitions, true),
    [UNIT] = {.accepting = true},
    [RIGHT_PARENTHESIS] = {.accepting = true},
    [DOUBLE_AMPERSAND] = {.accepting = true},
    [AMPERSAND] = OUTGOING(ampersand_transitions, false),
    [PIPE] = OUTGOING(pipe_transitions, false),
    [DOUBLE_PIPE] = {.accepting = true},
    [PLUS] = {.accepting = true},
    [MINUS] = {.accepting = true},
    [ASTERISK] = {.accepting = true},
    [SLASH] = OUTGOING(slash_transitions, true),
    [EMPTY_STRING] = OUTGOING(empty_string_transitions, true),
    [QUADRUPLE_QUOTE] = OUTGOING(quadruple_quote_transitions, false),
    [TRIPLE_QUOTE_TEXT_QUOTE] =
        OUTGOING(triple_quote_text_quote_transitions, false),
    [T_STRING_WITH_TEXT] = OUTGOING(t_string_with_text_transitions, false),
    [TRIPLE_QUOTE_TEXT_DOUBLE_QUOTE] =
        OUTGOING(triple_quote_text_double_quote_transitions, false),
    [TRIPLE_STRING_STATE] = {.accepting = true},
    [NUMBER_DOT] = OUTGOING(number_dot_transitions, false),
    [DOUBLE] = OUTGOING(double_transitions, true),
    [NUMBER_DOT_NUMBER_E] = OUTGOING(number_dot_number_e_transitions, false),
    [DOUBLE_WITH_EXPONENT] = OUTGOING(double_with_exponent_transitions, true),
    [UNDERSCORE_1] = OUTGOING(underscore_1_transitions, false),
    [UNDERSCORE_2] = OUTGOING(underscore_2_transitions, false),
    [UNDERSCORE_3] = OUTGOING(underscore_3_transitions, false),
    [DOUBLE_WITH_SIGNED_EXPONENT] =
        OUTGOING(double_with_signed_exponent_transitions, false),
    [PERIOD_STATE] = OUTGOING(period_state_transitions, true),
    [COMMENT] = OUTGOING(comment_transitions, false),
    [COMMA_STATE] = {.accepting = true},
    [COLON_STATE] = {.accepting = true},
    [SEMICOLON_STATE] = {.accepting = true},
    [AT] = OUTGOING(at_transitions, false),
    [LEFT_ARROW] = {.accepting = true},
    [UNDERSCORE_4] = OUTGOING(underscore_4_transitions, false),
    [EOL_OR_EOF] = {.accepting = true},
    [AT_M] = OUTGOING(at_m_transitions, false),
    [AT_MA] = OUTGOING(at_ma_transitions, false),
    [AT_MAI] = OUTGOING(at_mai_transitions, false),
    [AT_MAIN_STATE] = {.accepting = true},
    [ZERO] = OUTGOING(zero_transitions, true),
    [ESCAPED_1] = OUTGOING(escaped_1_transitions, false),
    [S] = OUTGOING(s_transitions, true),
    [DOUBLE_WITH_D_SUFFIX_1] = {.accepting = true},
    [DOUBLE_WITH_D_SUFFIX_2] = {.accepting = true},
    [SLASH_ASTERISK] = OUTGOING(slash_asterisk_transitions, false),
    [INSIDE_BLOCK_COMMENT] = OUTGOING(inside_block_comment_transitions, false),
    [SLASH_DOUBLE_ASTERISK_1] =
        OUTGOING(slash_double_asterisk_1_transitions, false),
    [POTENTIAL_NESTING] = OUTGOING(potential_nesting_transitions, false),
    [BLOCK_COMMENT] = {.accepting = true},
    [INSIDE_MULTILINE_BLOCK_COMMENT] =
        OUTGOING(inside_multiline_block_comment_transitions, false),
    [SLASH_DOUBLE_ASTERISK_2] =
        OUTGOING(slash_double_asterisk_2_transitions, false),
    [MULTILINE_BLOCK_COMMENT] = {.accepting = true},
    [NESTING] = OUTGOING(nesting_transitions, false),
    [S_QUOTE] = OUTGOING(s_quote_transitions, true),
    [S_DOUBLE_QUOTE] = OUTGOING(s_double_quote_transitions, false),
    [S_TRIPLE_QUOTE] = {.accepting = true},
    [DOLLAR_SIGN] = OUTGOING(dollar_sign_transitions, false),
    [QUOTE_1] = {.accepting = true},
    [QUOTE_2] = OUTGOING(quote_2_transitions, false),
    [DOUBLE_QUOTE] = OUTGOING(double_quote_transitions, false),
    [TRIPLE_QUOTE] = {.accepting = true},
    [DOLLAR_EXPRESSION_START_STATE] = {.accepting = true},
    [DOLLAR_EXPRESSION_END_STATE] = {.accepting = true},
    [TEXT_IN_STRING_2] = OUTGOING(text_in_string_2_transitions, true),
    [ESCAPED_2] = OUTGOING(escaped_2_transitions, false),
    [DOUBLE_DOLLAR_SIGN] = {.accepting = true},
    [TRIPLE_S_STRING_STRING] =
        OUTGOING(triple_s_string_string_transitions, true),
    [UNIT_WITH_WHITESPACE] = OUTGOING(unit_with_whitespace_transitions, false),
};

// END GENERATED LEXER TRANSITIONS

// AI generated by: Zed / GPT-6.1 Sol
// Prompt: "Implement [@lexer.c
// (797:804)](file:///home/daalbu/Code/IFJ26/src/lexer.c#L797:804) but change it
// to populate_token. It should also have a param which accepts a string for
// ident keyword matching. Token value should be filled at this stage. It needs
// to match [@lexer.txt](file:///home/daalbu/Code/IFJ26/lexer.txt) and [@lexer
// (10).json](file:///home/daalbu/Code/IFJ26/lexer%20(10).json) (this one should
// have the corresponding token names, it just lacks keyword detection). The
// token types are in TokenType enum in
// [@lexer.h](file:///home/daalbu/Code/IFJ26/src/lexer.h) ." Changes: Populate
// token types and values from accepted lexemes; false means skip. Text values
// are independent heap allocations owned by the token's caller.
static bool populate_token(Ctx* ctx,
                           Token* token,
                           State state,
                           const char* lexeme,
                           size_t length) {
  token->value.integer = 0;
  token->value_type = INTEGER;
  size_t prefix = 0;
  size_t suffix = 0;
  bool decode_escapes = false;

  switch (state) {
    case IDENTIFIER:
    case S: {
      // Interpolated identifiers include their leading '$' in the lexeme.
      if (length > 0 && lexeme[0] == '$') {
        lexeme++;
        length--;
      }
      static const struct {
        const char* word;
        TokenType type;
      } keywords[] = {
          {"import", IMPORT},
          {"def", DEF},
          {"val", VAL},
          {"var", VAR},
          {"return", RETURN},
          {"if", IF},
          {"then", THEN},
          {"else", ELSE},
          {"while", WHILE},
          {"do", DO},
          {"Int", TYPE_INT},
          {"Double", TYPE_DOUBLE},
          {"String", TYPE_STRING},
          {"Unit", TYPE_UNIT},
          {"for", FOR},
          {"Boolean", TYPE_BOOL},
          {"true", BOOL_TRUE},
          {"false", BOOL_FALSE},
      };
      for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        if (strlen(keywords[i].word) == length &&
            memcmp(lexeme, keywords[i].word, length) == 0) {
          token->type = keywords[i].type;
          token->value.integer = token->type == BOOL_TRUE;
          return true;
        }
      }
      token->type = IDENT;
      break;
    }
    case INTEGER_STATE:
    case ZERO: {
      int value = 0;
      for (size_t i = 0; i < length; i++) {
        if (lexeme[i] == '_')
          continue;
        int digit = lexeme[i] - '0';
        if (digit < 0 || digit > 9 || value > (INT_MAX - digit) / 10) {
          THROW(E_LEXER_ERROR, "Integer literal out of range");
        }
        value = value * 10 + digit;
      }
      token->type = NUMBER_INT;
      token->value.integer = value;
      return true;
    }
    case DOUBLE:
    case DOUBLE_WITH_EXPONENT:
    case DOUBLE_WITH_D_SUFFIX_1:
    case DOUBLE_WITH_D_SUFFIX_2: {
      char* number = TRY_MALLOC(length + 1);
      size_t count = 0;
      for (size_t i = 0; i < length; ++i) {
        if (lexeme[i] != '_' &&
            !(i + 1 == length && (lexeme[i] == 'd' || lexeme[i] == 'D'))) {
          number[count++] = lexeme[i];
        }
      }
      number[count] = '\0';
      char* end;
      errno = 0;
      double value = strtod(number, &end);
      bool invalid =
          end == number || *end != '\0' || errno == ERANGE || !isfinite(value);
      free(number);
      if (invalid)
        THROW(E_LEXER_ERROR, "Double literal out of range or invalid");
      token->type = NUMBER_DOUBLE;
      token->value_type = DECIMAL;
      token->value.decimal = value;
      return true;
    }
    case STRING_WITH_TEXT:
    case EMPTY_STRING:
      token->type = STRING;
      prefix = suffix = 1;
      decode_escapes = true;
      break;
    case TRIPLE_STRING_STATE:
      token->type = TRIPLE_STRING;
      prefix = suffix = 3;
      break;
    case TEXT_IN_STRING_2:
      token->type = STRING;
      decode_escapes = true;
      break;
    case TRIPLE_S_STRING_STRING:
      token->type = STRING;
      break;
    case DOUBLE_DOLLAR_SIGN:
      token->type = STRING;
      prefix = 1;
      break;
    case EQUALS:
      token->type = ASSIGN;
      return true;
    case ASSIGNMENT:
      token->type = COMPARISON_EQ;
      return true;
    case LESS_THEN:
      token->type = COMPARISON_L;
      return true;
    case LESS_THEN_OR_EQUAL:
      token->type = COMPARISON_LE;
      return true;
    case GREATER_THEN:
      token->type = COMPARISON_G;
      return true;
    case GREATER_THEN_OR_EQUAL:
      token->type = COMPARISON_GE;
      return true;
    case NEGATION:
      token->type = BOOL_NEG;
      return true;
    case NOT_EQUAL:
      token->type = COMPARISON_NEQ;
      return true;
    case LEFT_PARENTHESIS:
      token->type = L_PAREN;
      return true;
    // TokenType currently uses TYPE_UNIT for both Unit and its sole value ().
    case UNIT:
      token->type = TYPE_UNIT;
      return true;
    case RIGHT_PARENTHESIS:
      token->type = R_PAREN;
      return true;
    case DOUBLE_AMPERSAND:
      token->type = BOOL_AND;
      return true;
    case DOUBLE_PIPE:
      token->type = BOOL_OR;
      return true;
    case PLUS:
      token->type = OPERATOR_PLUS;
      return true;
    case MINUS:
      token->type = OPERATOR_MINUS;
      return true;
    case ASTERISK:
      token->type = OPERATOR_MULTIPLY;
      return true;
    case SLASH:
      token->type = OPERATOR_DIVIDE;
      return true;
    case PERIOD_STATE:
      token->type = PERIOD;
      return true;
    case COMMA_STATE:
      token->type = COMMA;
      return true;
    case COLON_STATE:
      token->type = COLON;
      return true;
    case SEMICOLON_STATE:
      token->type = SEMICOLON;
      return true;
    case LEFT_ARROW:
      token->type = CONTROL_ARROW;
      return true;
    case AT_MAIN_STATE:
      token->type = AT_MAIN;
      return true;
    case S_QUOTE:
      token->type = S_STRING_START;
      return true;
    case S_TRIPLE_QUOTE:
      token->type = TRIPLE_S_STRING_START;
      return true;
    case QUOTE_1:
      token->type = S_STRING_END;
      return true;
    case TRIPLE_QUOTE:
      token->type = TRIPLE_S_STRING_END;
      return true;
    case EOL_OR_EOF:
    case MULTILINE_BLOCK_COMMENT:
      token->type = EOL_TOKEN;
      return true;
    case DOLLAR_EXPRESSION_START_STATE:
      token->type = DOLLAR_EXPRESSION_START;
      return true;
    case DOLLAR_EXPRESSION_END_STATE:
      token->type = DOLLAR_EXPRESSION_END;
      return true;
    case WHITESPACE_STATE:
    case BLOCK_COMMENT:
      token->type = WHITESPACE;
      return false;
    default:
      THROW(E_INTERNAL_ERROR, "Cannot populate token from state %d", state);
      return false;
  }

  if (length < prefix + suffix) {
    THROW(E_INTERNAL_ERROR, "Incomplete string token");
  }
  size_t end = length - suffix;
  char* text = TRY_MALLOC(end - prefix + 1);
  size_t count = 0;
  for (size_t i = prefix; i < end; ++i) {
    char character = lexeme[i];
    if (decode_escapes && character == '\\') {
      if (++i == end) {
        free(text);
        THROW(E_LEXER_ERROR, "Incomplete escape sequence");
      }
      switch (lexeme[i]) {
        case 'n':
          character = '\n';
          break;
        case 'r':
          character = '\r';
          break;
        case 't':
          character = '\t';
          break;
        case '"':
          character = '"';
          break;
        case '\\':
          character = '\\';
          break;
        default:
          free(text);
          THROW(E_LEXER_ERROR, "Invalid escape sequence");
      }
    }
    text[count++] = character;
  }
  text[count] = '\0';
  token->value_type = TEXT;
  token->value.text = text;
  return true;
}

/**
 * @brief Helper function for finding next transition if available
 *
 * @param state current state of the lexer
 * @param character current character being processed
 * @param top top of the stack
 * @return pointer to the specific transition if found, NULL otherwise
 */
static const Transition* find_transition(int state, int character, int top) {
  StateTransitions current_state = state_transitions[state];
  for (size_t i = 0; i < current_state.count; i++) {
    const Transition* transition = &current_state.transitions[i];
    if (transition->matches(character) &&
        (TOP(top) & transition->stack_top_mask)) {
      return transition;
    }
  }
  return NULL;
}

/**
 * @brief Copies the contents of a stack from source to destination
 *
 * @param ctx[in] context
 * @param destination[out] destination stack
 * @param source[in] source stack
 */
static void copy_stack(Ctx* ctx,
                       LexerStack* destination,
                       const LexerStack* source) {
  array_reserve(ctx, destination, source->count);
  for (size_t i = 0; i < source->count; ++i) {
    destination->data[i] = source->data[i];
  }
  destination->count = source->count;
}

/**
 * @brief provides next token from the stdin stream
 *
 * @param ctx context of the compiler
 * @return next token from the stdin stream
 */
Token next_token(Ctx* ctx) {
  // buffer for pushback
  LexerByteBuffer* buffer = &ctx->lexer.buffer;

  // initialized token
  Token token = {.type = EOF_TOKEN,
                 .value = {.integer = 0},
                 .value_type = INTEGER,
                 .line = ctx->lexer.line,
                 .column = ctx->lexer.column};

  State current_state = START;
  State saved_state = START;
  size_t saved_cursor = 0;
  size_t saved_line = 0;
  size_t saved_column = 0;
  bool have_checkpoint = false;

  LexerStack* stack = &ctx->lexer.stack;
  LexerStack* saved_stack = &ctx->lexer.saved_stack;
  // Initialize stack if empty
  if (stack->count == 0) {
    array_append(ctx, stack, STACK_EMPTY);
  }

  size_t token_start = ctx->lexer.cursor;

  while (true) {
    int current;

    // fetching next char from buffer if possible, otherwise save new to buffer
    if (buffer->count > ctx->lexer.cursor) {
      current = buffer->data[ctx->lexer.cursor];
      ctx->lexer.cursor++;
    } else {
      current = getchar();
      if (current >= 0) {
        array_append(ctx, buffer, current);
        ctx->lexer.cursor++;
      } else {
        // EOF - no more characters
      }
    }

    // top of the stack
    int top = stack->count == 0 ? STACK_EMPTY : stack->data[stack->count - 1];

    const Transition* transition = find_transition(current_state, current, top);
    if (transition == NULL) {
      if (current == EOF) {
        // return EOF if it is the last token
        if (current_state == START && ctx->lexer.cursor == token_start &&
            stack->count == 1 && top == STACK_EMPTY) {
          token.type = EOF_TOKEN;
          return token;
        }
        if (stack->count != 1 && top != STACK_EMPTY) {
          THROW(E_LEXER_ERROR, "unfinished token");  // TODO: better error
        }
        if (current_state == SLASH_ASTERISK ||
            current_state == INSIDE_BLOCK_COMMENT ||
            current_state == INSIDE_MULTILINE_BLOCK_COMMENT ||
            current_state == SLASH_DOUBLE_ASTERISK_1 ||
            current_state == SLASH_DOUBLE_ASTERISK_2 ||
            current_state == POTENTIAL_NESTING || current_state == NESTING) {
          THROW(E_LEXER_ERROR, "unfinished token");  // TODO: better error
        }
        // line comment ends with eof
        if (current_state == COMMENT) {
          if (populate_token(ctx, &token, EOL_OR_EOF,
                             (const char*)buffer->data + token_start,
                             saved_cursor - token_start)) {
            return token;
          }
        }
        // return the token if it is accepting before EOF
        if (state_transitions[current_state].accepting) {
          if (populate_token(ctx, &token, current_state,
                             (const char*)buffer->data + token_start,
                             saved_cursor - token_start)) {
            return token;
          }
        }
      }
      if (have_checkpoint) {
        ctx->lexer.cursor = saved_cursor;
        ctx->lexer.line = saved_line;
        ctx->lexer.column = saved_column;
        current_state = saved_state;
        copy_stack(ctx, stack, saved_stack);

        if (populate_token(ctx, &token, current_state,
                           (const char*)buffer->data + token_start,
                           saved_cursor - token_start)) {
          return token;
        }
        current_state = START;
        saved_state = START;
        have_checkpoint = false;
        token_start = ctx->lexer.cursor;
        token.line = ctx->lexer.line;
        token.column = ctx->lexer.column;
        continue;
      }

      THROW(E_LEXER_ERROR,
            "Invalid or incomplete token");  // TODO: add line and column
    } else {
      switch (transition->stack_action) {
        case TRANSITION_KEEP:
          break;
        case TRANSITION_PUSH:
          array_append(ctx, stack, transition->stack_symbol);
          break;
        case TRANSITION_POP:
          array_remove(ctx, stack, stack->count - 1);  // remove last
          break;
        case TRANSITION_REPLACE:
          array_remove(ctx, stack, stack->count - 1);  // remove last
          array_append(ctx, stack, transition->stack_symbol);
          break;
      }

      current_state = transition->next_state;

      // update last_accepting state, save cursor and stack
      // when going to accepting state
      if (state_transitions[current_state].accepting) {
        saved_state = current_state;
        // save cursor
        saved_cursor = ctx->lexer.cursor;
        // save line and column
        saved_line = ctx->lexer.line;
        saved_column = ctx->lexer.column;
        // save stack
        copy_stack(ctx, saved_stack, stack);
        have_checkpoint = true;
      }
    }
  }
}
