# 03: Blocks and simple statements

**What to build:** Function bodies on their own indented lines parse into BLOCKs of statements: `val`/`var` declarations, assignments, `return`, expression statements, and several statements on one line separated by `;`. Spec: `.scratch/parser-ast-and-handoff/spec.md`.

**Blocked by:** 01 (Tracer: the smallest program parses end to end).

**Status:** resolved

- [ ] Nonterminal functions for the multi-line function body, BLOCK (block open, statements, block close), the statement list, statement, statement tail, base statements, the optional variable type, the return value and the right-hand side value (expression form).
- [ ] VAL and VAR are `[ident, type|NONE, init]`; RETURN is `[expr|NONE]`; ASSIGN is `[ident, expr]`.
- [ ] `parser_peek` is implemented; the peeked token passes through the token layer like any other (WHITESPACE dropped, raw indents translated).
- [ ] A statement starting with an identifier peeks for `=`: an assignment consumes the identifier and `=`; otherwise `expr_parse` starts at the identifier and the expression node itself becomes the statement. Statements starting with a literal, `(`, `-` or `!` go straight to `expr_parse`.
- [ ] A statement after `;` is appended as the next sibling in the enclosing BLOCK.
- [ ] Tests: a body like `sumTo`'s without the loop, `val`/`var` with and without type annotations, `return` with and without a value, an assignment, an expression statement, `a; b` on one line, a nested block's statements in order, and syntax errors (missing `=` after `val x`, a stray token in a block) with no leaks.

## Comments

- 2026-11-16 (implement-spec): Implemented in 3636f25 (merge b38a6a1). Review fix c7f1b36: block open vs block close told apart (a deeper line was accepted as a block close). `-x`/`!x` at statement start are rejected, following the LL table (STATEMENT_BASE has no such rule); the ticket text says otherwise, to be settled by the parser author. Integrated on `feat/implement-parser`; final review fixes e7d531b (no leak when an allocation fails, ADR-0004), 13cffaf.
