# Parsing builds the AST; semantic analysis is a separate pass

The recursive descent parser, driven by the LL table, does syntax analysis only (exit codes 1 and 2) and builds the statement-level AST. It hands every expression to a separate precedence parser, which builds the expression subtree and returns it. Semantic analysis (exit codes 3 to 10) runs later, over the finished AST, and is not part of the parser.

## Considered Options

- **Semantic checks during the parse (one pass).** Rejected: IFJ26 lets a function be called before its definition (`tests/e2e/everything`: `run` calls `trace` and `pack`, which are defined later) and lets return types be inferred (`def empty() = ()`). A single pass would need a pre-pass that collects signatures, or deferred checks, in addition to the parser.
- **Verify first, then build the whole AST with the precedence parser.** Rejected: a precedence parser cannot build statements (`def`, `while`, blocks), and reading the tokens twice adds a second traversal for nothing.
