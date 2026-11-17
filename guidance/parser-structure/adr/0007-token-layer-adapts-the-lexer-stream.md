# The token layer adapts the lexer's actual token stream

The lexer on `feat/implement-lexer` (`Token next_token(Ctx* ctx)`, state in `ctx->lexer`) emits a stream that differs from the contract ADR-0005 and ADR-0006 assumed. The grammar stays as it is. The token layer (`fetch_token` in `parser.c`) turns the real stream into the one the grammar expects:

- **`()` is the UNIT token.** The grammar's `TYPE_UNIT` stands for both `Unit` and `()`, so the token layer reads UNIT as `TYPE_UNIT`.
- **EOL_TOKEN and INDENT also appear inside parentheses.** While a `(` is open, the token layer hides them, unless the innermost open construct is the text of an interpolated string (more string starts than `${` are open). It records that a line break came before the next token it delivers. A trailing comma checks that record, not line numbers, because the lexer gives every token line 1 so far (amends ADR-0005).
- **Blank and comment-only lines emit an EOL_TOKEN, and an indented one also emits a raw indent first.** An EOL_TOKEN that starts a line is hidden. A raw indent followed by an EOL_TOKEN or by the end of file decides nothing (amends ADR-0006).
- **A last line without `\n` ends with no EOL_TOKEN.** At the end of file, if the last line is still open, the token layer delivers an EOL_TOKEN first. A program whose last line has no line break is therefore accepted.
- **The lexer never returns WHITESPACE.** It drops whitespace and block comments itself, so the token layer does not skip WHITESPACE.
- **Token text is heap memory exactly when `value_type` is TEXT.** The interpolated string delimiters carry no text, so ownership follows `value_type`, not the token type (refines ADR-0002).

## Considered Options

- **Ask the lexer to follow the old contract.** Rejected for now: the lexer belongs to another team member. Every adaptation above is local to the token layer and becomes a no-op if the lexer changes.
- **Hide a line break inside a triple-quoted interpolated string as well.** Rejected: there the lexer emits an EOL_TOKEN for a newline that is string content. Hiding it would silently drop the `\n` from the string. That is a lexer bug, and the parser reports it as a syntax error, inside parentheses too.

## Merging the lexer branch

The lexer branch is now merged. `ctx.h` and `ctx.c` keep both sides: `Parser* parser` with the `parser_cleanup(ctx)` call in `ctx_free` (the parser's state, freed after a THROW), and `LexerCtx lexer`, whose arrays `ctx_free` releases. `main` in `ifj26.c` calls `parser_parse`, which reads stdin through `next_token`.
