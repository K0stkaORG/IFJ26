# 05: If statements and if expressions

**What to build:** Every form of `if` parses: multi-line and single-line branches, `else if`, the `;` form, and if expressions in value positions (right side of `val`/`var`/assignment/`return`, call arguments, parentheses), with the EXPR flag set only on those. Spec: `.scratch/parser-ast-and-handoff/spec.md`.

**Blocked by:** 03 (Blocks and simple statements), 04 (Calls).

**Status:** resolved

- [ ] Nonterminal functions for the if statement, its body, else part, inline else part, else body, the single-line body (return, assignment via peek, expression), its return value and identifier forms, the if-expression form of the inline value, and the right-hand side `if` form.
- [ ] IF is `[cond, then BLOCK, else BLOCK | IF | NONE]`; single-line branches are wrapped in a BLOCK; `else if` is an IF as the else child.
- [ ] The EXPR flag is set on IFs built from the inline if and from the right-hand side `if` form, and nowhere else; a missing `else` is never a syntax error by itself.
- [ ] `(if … then … else …)` inside an expression: the precedence parser calls back into the inline if with a NONE holder node from its stack as parent, then detaches the IF and uses it as the operand.
- [ ] `if c then x; y` puts `y` after the IF in the enclosing BLOCK.
- [ ] Tests: `classify` (else-if chain of single-line bodies), `chooseDouble` (`val result = if … then` block `else 2.5`), `isEven` (single-line body, no EXPR flag), `guardedDivide`, `factorial`'s `if n <= 1 then return 1`, `joined` (multi-line branches), `if truth then print(";if")`, an if expression as a call argument, `(if a then 1 else 2) + 3`, and syntax errors (`if c print(x)`, a missing `then`) with no leaks.

## Comments

- 2026-11-16 (implement-spec): Implemented in 7ff49e7 (merge 6478e88), review fixes b54efa0. Open question for the parser author: in `val a = if … else if …` only the outer IF carries EXPR (literal spec), while `(if … else if …)` flags both. `(if c then 1)` without else is a syntax error (IF_INLINE requires ELSE in the grammar). Integrated on `feat/implement-parser`; final review fixes e7d531b (no leak when an allocation fails, ADR-0004), 13cffaf.
