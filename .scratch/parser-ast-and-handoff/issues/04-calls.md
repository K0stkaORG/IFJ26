# 04: Calls

**What to build:** Calls to builtins and user functions parse anywhere an operand may appear, including as statements and nested as arguments, through the ARG_LIST callback from the precedence parser. Spec: `.scratch/parser-ast-and-handoff/spec.md`.

**Blocked by:** 01 (Tracer: the smallest program parses end to end).

**Status:** ready-for-agent

- [ ] An identifier followed by `(` or the unit token is a call: the precedence parser builds a CALL node, pushes it onto its stack, and calls back into ARG_LIST with the CALL as parent (same signature as every nonterminal function). `f()` gives a CALL with no arguments.
- [ ] Nonterminal functions for the argument list, its tail and the inline value (expression form); each argument is a full value parsed with its own frame on the shared precedence stack.
- [ ] A trailing comma before `)` is accepted.
- [ ] The callback prototypes live in the private parser header.
- [ ] Tests: `readInt()`, `print(x)`, `pack(trace("L", n), readInt(), trace("R", n),)`, a call as a statement, a call as a `val` initializer, nested calls, and a syntax error inside an argument list (`f(a b)`, `f(a,,b)`) with no leaks. If ticket 02 has landed, add a call with operator arguments (`trace("R", n + 2)`).
