#include "ast.h"
#include "lexer.h"

#ifndef PARSER_H
#define PARSER_H

typedef struct Parser Parser;

typedef enum {
  PARSER_OK,
  PARSER_ERROR,
} ParserStatus;

Parser* parser_create(void);
void parser_destroy(Parser* parser);
ParserStatus parser_parse(Parser* parser, AstNode** root);

#endif
