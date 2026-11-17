# A trailing comma needs a line break before the `)`

A trailing comma in a parameter or argument list is accepted only when a line break comes between it and the closing `)`. `print(1,)` is a syntax error (exit 2), and `print(1,` followed by `)` on the next line is accepted. That is what PDF section 1.2 says: a trailing comma is allowed only if the next token is the closing parenthesis and an end of line lies between them.

The grammar is unchanged (`ARG_NEXT -> ε` and `PARAM_NEXT -> ε` still take the `)`). The parser compares line numbers when it consumes the comma (`parse_list_comma`): if the next token is `)` on the comma's line, that is a syntax error. This assumes the lexer emits no EOL token inside parentheses and gives every token its source line. In unit tests the fake token source numbers lines itself, and `BREAK` marks a line break inside parentheses.

This amends parser story 43, which accepted `pack(a, b, c,)` on one line.

## Considered Options

- **Have the lexer report the line break as a token.** Rejected: the lexer is outside the modules this work may change, and the grammar would need that token in every list.
- **Keep accepting any trailing comma.** Rejected: it does not match the PDF.
