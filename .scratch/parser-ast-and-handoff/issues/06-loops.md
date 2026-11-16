# 06: Loops

**What to build:** `while` and `for` loops parse, including ranges with `to`, `until` and `by`, inline and multi-line guards, and single-line or block bodies; a misspelled contextual keyword is a syntax error. Spec: `.scratch/parser-ast-and-handoff/spec.md`.

**Blocked by:** 02 (Operators in expressions), 05 (If statements and if expressions).

**Status:** ready-for-agent

- [ ] The AST module's RANGE_UNTIL layout becomes `[from, until, step|NONE]` (matching RANGE_TO); its layout comment and any consumers are updated.
- [ ] Nonterminal functions for the while and for statements, the for header (single-line and multi-line forms), the generator, the range step, the guard list, the guard lines and the loop body.
- [ ] WHILE is `[cond, BLOCK]`; FOR is `[ident, RANGE_TO | RANGE_UNTIL, BLOCK, guard...]`; single-line loop bodies are wrapped in a BLOCK.
- [ ] Contextual keywords are checked by their text: `to` builds RANGE_TO, `until` builds RANGE_UNTIL, `by` adds the step; any other word there is a syntax error.
- [ ] Tests: `sumTo`'s while loop, a single-line while body with `return`, every loop of `ranges()` in the `everything` e2e program (including the multi-line header with two guards, `6 to 2 by -2`, `2 until 5`, and an `until … by` range), and syntax errors (`for i <- 1 tu 5 do …`, `for i <- 1 to 5 bye 2 do …`, a missing `do`) with no leaks.
