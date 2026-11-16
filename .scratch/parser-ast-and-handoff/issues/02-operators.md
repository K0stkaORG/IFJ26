# 02: Operators in expressions

**What to build:** Expressions with operators, parentheses and member access parse into correctly shaped trees, following the precedence table and its reduction rules, and an error in the middle of an expression frees every partly built subtree. Spec: `.scratch/parser-ast-and-handoff/spec.md`.

**Blocked by:** 01 (Tracer: the smallest program parses end to end).

**Status:** ready-for-agent

- [ ] Tokens map to precedence terminals as the spec lists; reductions build ADD, SUB, MUL, DIV, EQ, NEQ, LT, GT, LE, GE, AND, OR from the shifted operator token, NEG and NOT for `pre`, MEMBER (name = member) for `.m`; `( E )` leaves no node.
- [ ] `-` is `pre` unless it follows an operand or `)`; `!` is always `pre`; `-7` is NEG of INT_LIT 7 (no folding).
- [ ] Expression end: a token that is not a precedence terminal, a `)` with no `(` of this expression open, or an operand-starting token right after a complete operand or `)`; the end token stays current. An empty table cell or failed reduction is a syntax error at the current token.
- [ ] Operator nodes carry the operator token's position.
- [ ] Tests: `40 - n * 3 - 2`, `(40 - n) * (3 - 2)`, `-(-n)`, `a - -b`, `!x && y`, `a && b || c`, `n == 4.0 && "ab" == "a" + "b"`, `(-3.75).toInt.toString`, `word.length`, and the chained comparison/logic line from the `everything` e2e program.
- [ ] Tests: syntax errors with a non-empty precedence stack (`1 + * 2`, `(a + b`, `a +` at end of line) give exit code 2 with no leaks under valgrind.
