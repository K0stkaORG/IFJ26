# The precedence stack lives in the parser state and owns its subtrees

Every AST node the recursive descent creates is attached to its parent at once, so `ctx_free` frees a partly built tree after a `THROW`. The precedence parser cannot do that: its stack holds subtrees that have no parent yet. So the precedence stack is one array in `Parser`, not a local of `expr_parse`. Each `expr_parse` call, including the nested ones made through callbacks, uses only the part of the stack above where it started, and `parser_cleanup` destroys the subtrees still on it. A syntax error in the middle of an expression therefore leaks nothing.

Callbacks keep the one signature every nonterminal function has (they attach to a parent). The precedence parser pushes the node it builds (`CALL`, `INTERP_STRING`) onto its stack before calling back with it as the parent. For `(if …)`, which has no natural parent, it passes a `NONE` holder node from its stack and detaches the `IF` from it afterwards.

## Considered Options

- **A local stack per `expr_parse` call.** Rejected: its subtrees leak when a syntax error is thrown, which `make test-memory` reports.
- **Callbacks that return a node.** Rejected: a second convention next to the parent-attaching template.
