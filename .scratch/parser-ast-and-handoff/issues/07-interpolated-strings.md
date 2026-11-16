# 07: Interpolated strings

**What to build:** Interpolated strings (`s"…"` and `s"""…"""`) parse into an INTERP_STRING node whose parts are text and values in source order, using the lexer's `${…}` tokens. Spec: `.scratch/parser-ast-and-handoff/spec.md`.

**Blocked by:** 02 (Operators in expressions), 04 (Calls). Also requires the parser author to adopt the table maker draft "Interpolated strings: ${…} uses the lexer's DOLLAR_EXPRESSION_START/END tokens" (human step).

**Status:** ready-for-agent

- [ ] The token type enum gains `DOLLAR_EXPRESSION_START` and `DOLLAR_EXPRESSION_END` right after `TRIPLE_S_STRING_END`, as on the lexer branch, with names for syntax error messages.
- [ ] The AST module gains the INTERP_STRING node kind with layout `[part...]`, each part a STRING_LIT or any expression; its layout comment is added.
- [ ] An interpolated string is an operand: the precedence parser builds the INTERP_STRING, pushes it onto its stack, and adds parts: STRING tokens (including `$$`) as STRING_LIT, `$name` as IDENT, and `${…}` through the inline value callback with the INTERP_STRING as parent. `s""` gives an INTERP_STRING with no children.
- [ ] The closing `}` ends the inner expression (it is not a precedence terminal).
- [ ] Tests: the `interpolateds` e2e program's two strings, `s"|$packed\n"`, `${returnedCall(n)}`, `${count + 1}`, `$$` followed by `${…}`, an empty interpolated string, an interpolated string inside a larger expression (`"a" + s"$b"`), and a syntax error inside `${…}` with no leaks.
