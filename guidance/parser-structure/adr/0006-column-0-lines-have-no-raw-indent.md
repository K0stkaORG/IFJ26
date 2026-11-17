# A line in column 0 has no raw indent

The lexer emits a raw indent (an INDENT token carrying the line's indentation) only at the start of an indented line. A line in column 0 starts directly with its first token. The token layer therefore treats the first token after an EOL_TOKEN that is not an INDENT as indentation 0: it closes every open block, one block close per level, and then delivers that token, which it holds back meanwhile. The end of file closes every open block the same way.

This needs one guarantee from the lexer: every source line ends with an EOL_TOKEN, so the token layer knows the next token starts a line. Blank and comment-only lines still emit nothing; an EOL_TOKEN at the start of a line is ignored by this rule rather than taken for a line in column 0. The grammar is unchanged: the parser still sees only block open / block close INDENT tokens, and a block close is still the parser's own INDENT with value 0.

## Considered Options

- **Have the lexer emit INDENT with value 0 for lines in column 0.** Rejected: the lexer does not do this, and it is outside the modules this work may change.
- **Have the lexer emit DEDENT tokens.** Rejected for the same reason; if it ever does, only the token layer's translation changes.

Amended by ADR-0007: blank and comment-only lines do emit an EOL_TOKEN (and an indented one a raw indent first); the token layer hides them, and supplies the EOL_TOKEN of a last line without a line break.
