# 01: Tracer: the smallest program parses end to end

**What to build:** A whole program made of the prolog and single-line functions whose body is one operand parses from a token array into the AST in the shared context, or stops with a syntax error (exit code 2) that leaks nothing. This lays down the test seam and the precedence stack every later ticket builds on. Spec: `.scratch/parser-ast-and-handoff/spec.md`.

**Blocked by:** None (can start immediately).

**Status:** ready-for-agent

- [ ] An internal entry point parses a whole program from any token source; the public parser entry is a thin wrapper over it with the lexer adapter. The PROGRAM root is stored in the context before parsing starts.
- [ ] A test-only printer renders an AST as a compact S-expression (node kind, name or value, flags, children); parser tests compare its output.
- [ ] Nonterminal functions exist for the program structure, prolog, function list, `@main` (with and without a line break before `def`), parameters, types, return type and the single-line function body; each follows the template (predict-set switch, attach to parent at once, syntax error listing expected terminals).
- [ ] The prolog's identifier must be exactly `ifj26`; any other word is a syntax error. The prolog produces no node.
- [ ] Every DEF is `[ident, rettype|NONE, BLOCK, PARAM_LIST]`, with the MAIN flag for `@main`; `def f()` (unit token) gives an empty PARAM_LIST; parameters are PARAM `[ident, type annotation]`.
- [ ] The precedence stack lives in the parser state, each expression parse uses its own frame, and parser cleanup destroys every subtree left on it (ADR-0004). `expr_parse` handles a single operand: identifier, literal (int, double, string, triple string, bool) or `()` as UNIT_LIT, stopping at the expression end.
- [ ] Single-line function bodies are wrapped in a BLOCK.
- [ ] Every node carries the line and column of the token it starts at.
- [ ] Tests: `def f(a: Int): Int = a`, a `@main` function, an inferred return type (`def empty() = ()`), several functions in order, a wrong prolog, and a syntax error inside a parameter list, all under valgrind with no leaks.
