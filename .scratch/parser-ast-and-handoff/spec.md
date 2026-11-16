# Parser: building the AST and handing expressions to the precedence parser

Status: resolved

Vocabulary follows `guidance/parser-structure/GLOSSARY.md`. Decisions are recorded in `guidance/parser-structure/adr/` (ADR-0001 to ADR-0004).

## Problem Statement

The parser of the IFJ26 compiler has its structure and token layer in place, but it does not parse anything yet: the start nonterminal function is never called, `parser_peek` is a stub and the precedence parser is empty. The LL table and the precedence table are finished (in the IFJ26 Table Maker project), but nothing turns a token stream into the AST that semantic analysis, the optimizer and code generation are waiting for. Several things the code needs were undecided until now: what AST each construct becomes, where an expression starts and ends, how the recursive descent and the precedence parser call each other, and how partly built expression subtrees are freed when a syntax error is thrown.

## Solution

The parser reads the whole program and leaves its AST in the shared context, or reports a syntax error (exit code 2) and frees everything. The recursive descent implements every nonterminal function of the LL table except the expression nonterminals; every expression is handed to the precedence parser, which builds the expression's subtree and calls back into three nonterminal functions for the parts of operands that contain full values. The resulting AST follows the child layouts documented in the AST module, extended by two small changes (an interpolated string node and a step slot for `until` ranges).

## User Stories

1. As the parser author, I want the parser to read a whole program from any token source and leave the AST in the shared context, so that semantic analysis can start from `ctx->ast`.
2. As the parser author, I want parsing to start at the start nonterminal of the LL table, so that the table stays the single source of truth for the grammar.
3. As the parser author, I want one nonterminal function per non-expression nonterminal of the LL table, each switching on the predict sets of its rules, so that the code can be checked rule by rule against the table.
4. As the parser author, I want the expression nonterminals to have no nonterminal functions, so that there is exactly one parser for expressions (ADR-0003).
5. As a compiler user, I want a program whose tokens the LL table has no production for to stop compilation with exit code 2, so that I learn my program is syntactically wrong.
6. As a compiler user, I want the syntax error message to name the unexpected token and what was expected, so that I can fix the program.
7. As a teammate running `make test-memory`, I want a syntax error anywhere, including in the middle of an expression, to leak nothing, so that memory tests stay green.
8. As the parser author, I want the prolog `import ifj26.*` to be checked, including that the identifier is exactly `ifj26`, so that `import foo.*` is a syntax error.
9. As the semantic analysis author, I want the program to be a PROGRAM node whose children are the DEF nodes in source order, so that I can collect every function before checking calls (functions may be called before their definition).
10. As the semantic analysis author, I want a function annotated with `@main` (with or without a line break before `def`) to carry the MAIN flag, so that I can find the entry point.
11. As the semantic analysis author, I want every DEF to have exactly one PARAM_LIST, empty for `def f()`, so that parameter handling is uniform.
12. As the semantic analysis author, I want each parameter to be a PARAM with its identifier and its type annotation, so that I can declare parameters in the function's scope.
13. As the semantic analysis author, I want an omitted return type to be a NONE placeholder, so that I know to infer it (`def empty() = ()`).
14. As the semantic analysis author, I want every function body to be a BLOCK, including single-line bodies (`def pack(…) = expr` and `def isEven(…) = if …`), so that body handling is uniform.
15. As the semantic analysis author, I want indented blocks to become BLOCK nodes with their statements in order, so that scopes match the source.
16. As the semantic analysis author, I want `val` and `var` to be VAL and VAR nodes with identifier, optional type annotation (NONE when omitted) and initializer, so that I can declare variables and check writes to `val`.
17. As the semantic analysis author, I want `x = e` to be an ASSIGN node, so that I can check the target exists and is writable.
18. As the semantic analysis author, I want `return` with and without a value to be a RETURN node with the value or NONE, so that I can check it against the return type.
19. As the semantic analysis author, I want an expression used as a statement (a call, `x`, `1 + 2`, `(-3.75).toInt`) to be the expression node itself in the BLOCK, so that the block's value is its last statement's.
20. As a compiler user, I want `a; b` on one line to mean two statements, so that I can write `classify(-n); classify(zero)`.
21. As the semantic analysis author, I want the statement after `;` to be the next sibling in the enclosing BLOCK, never part of a `then` branch (`if c then x; y` puts `y` after the IF), so that the AST matches Scala's meaning.
22. As the semantic analysis author, I want `if` to be an IF node with condition, then-BLOCK and else (a BLOCK, an IF for `else if`, or NONE), so that branches are uniform.
23. As the semantic analysis author, I want single-line `then` and `else` bodies wrapped in a BLOCK, so that I never special-case them.
24. As the semantic analysis author, I want an if expression (inside parentheses, a call argument, `${…}`, or the right side of `val`, `var`, an assignment or `return`) to carry the EXPR flag, so that I can require its `else` and give it a type.
25. As the semantic analysis author, I want an `if` that merely ends a body to carry no EXPR flag, so that I decide from the (possibly inferred) return type whether its value is used.
26. As the parser author, I want a missing `else` never to be a syntax error by itself, so that that check stays in semantic analysis.
27. As the semantic analysis author, I want `while c do …` to be a WHILE node with condition and BLOCK body, single-line bodies included.
28. As the semantic analysis author, I want `for i <- a to b by s` to be a FOR node whose source is RANGE_TO with the step or NONE, so that I can type the loop variable.
29. As the semantic analysis author, I want `for i <- a until b` and `for i <- a until b by s` to be a RANGE_UNTIL with the step or NONE, so that `until … by` is representable.
30. As the semantic analysis author, I want every guard of a `for` (single-line `if` guards and the multi-line header form) to be a child of the FOR after its body, in source order.
31. As a compiler user, I want a misspelled contextual keyword (`for i <- 1 tu 5`, `… by` written as another word) to be a syntax error, so that typos are caught early.
32. As the semantic analysis author, I want identifiers to be IDENT nodes with their name and no symbol yet, so that I resolve them myself.
33. As the semantic analysis author, I want literals to be INT_LIT, DOUBLE_LIT, STRING_LIT and BOOL_LIT carrying the value the lexer produced, and `()` to be UNIT_LIT, so that I never re-read token text.
34. As the semantic analysis author, I want triple-quoted strings to be STRING_LIT like any other string.
35. As the semantic analysis author, I want an interpolated string to be an INTERP_STRING node whose parts are STRING_LIT nodes and expressions in source order (`$name` an IDENT, `${…}` its value's subtree, `$$` a STRING_LIT), so that converting parts to text stays with later passes.
36. As the semantic analysis author, I want an empty interpolated string to be an INTERP_STRING with no children.
37. As the semantic analysis author, I want binary operators to become ADD, SUB, MUL, DIV, EQ, NEQ, LT, GT, LE, GE, AND, OR with left and right operand, so that I can type them.
38. As a compiler user, I want operator precedence and associativity to follow the precedence table (`40 - n * 3 - 2` is `(40 - (n * 3)) - 2`, `a && b || c` is `(a && b) || c`), so that programs compute what I wrote.
39. As a compiler user, I want parentheses to group without leaving a node in the AST, so that `(40 - n) * (3 - 2)` is MUL of two SUBs.
40. As the semantic analysis author, I want unary minus to be NEG and `!` to be NOT, with `-7` as NEG of INT_LIT 7 (no folding in the parser), so that the optimizer owns folding.
41. As a compiler user, I want `-` after an operand or `)` to be subtraction and anywhere else to be unary minus, so that `-(-n)`, `a - -b` and `6 to 2 by -2` all parse.
42. As the semantic analysis author, I want every call to be a CALL node with callee and arguments, `f()` with zero arguments, so that builtins and user functions look alike.
43. As a compiler user, I want a trailing comma in an argument list to be accepted (`pack(a, b, c,)`), as the grammar allows.
44. As a compiler user, I want call arguments to be full values, including if expressions and nested calls.
45. As the semantic analysis author, I want `receiver.name` to be a MEMBER node with the receiver and the member name, chained left to right (`(-3.75).toInt.toString`), so that I can resolve builtin members.
46. As a compiler user, I want an expression to end where the grammar says the statement continues (`then`, `do`, `else`, `if` guards, `to`, `until`, `by`, end of line, `,`, `;`, `)` closing a call, end of `${…}`), so that headers like `while n > 0 do` and `for i <- 1 to 5 if i != 3` parse.
47. As the parser author, I want a statement starting with an identifier to peek for `=` and otherwise hand the whole expression, identifier included, to the precedence parser, so that calls are parsed in one place only.
48. As the parser author, I want `parser_peek` implemented so that the peeked token goes through the same token layer translation as every other token.
49. As the parser author, I want the precedence parser to call back into the ARG_LIST, INLINE_VALUE and IF_INLINE nonterminal functions with the same signature as every nonterminal function, so that there is one convention.
50. As the parser author, I want `(if … then … else …)` inside an expression to work through a holder node that is detached afterwards, so that IF_INLINE needs no special signature.
51. As the parser author, I want nested expressions (arguments, `${…}`, parenthesised if expressions) to share one precedence stack, each using its own part of it, so that one cleanup frees them all (ADR-0004).
52. As the semantic analysis author, I want every node to carry the line and column of the token that starts it (operators: the operator token), so that my error messages point at the source.
53. As the parser author, I want the parser to own and free every token it receives and never let the AST point at token text (ADR-0002).
54. As a teammate merging the lexer, I want the parser to use the lexer's `DOLLAR_EXPRESSION_START` and `DOLLAR_EXPRESSION_END` tokens for `${…}`, so that the two modules agree.

## Implementation Decisions

- **Prerequisite.** The table maker draft "Interpolated strings: ${…} uses the lexer's DOLLAR_EXPRESSION_START/END tokens" must be adopted by the parser author before implementation, so that the project's LL table has `INTERP_PART -> STRING | IDENT | DOLLAR_EXPRESSION_START INLINE_VALUE DOLLAR_EXPRESSION_END`. The implementer reads the grammar, LL table, predict sets, precedence table and reduction rules from the table maker project and implements them as they are. Agents never design or change the grammar, the LL table or the precedence table (AGENTS.md).
- **Token types.** The two tokens `DOLLAR_EXPRESSION_START` and `DOLLAR_EXPRESSION_END` are added to the parser branch's token type enum in the same place they have on the lexer branch (right after `TRIPLE_S_STRING_END`), so that the lexer merge is clean. Their token names for syntax error messages are added too. `()` and the type `Unit` stay one token (`TYPE_UNIT`) until the lexer splits them (see Out of Scope).
- **AST module changes (minimal).** One new node kind, INTERP_STRING, with layout `[part...]`, where each part is a STRING_LIT or any expression. RANGE_UNTIL changes from `[from, until]` to `[from, until, step|NONE]`, matching RANGE_TO. Layout comments in the AST header are updated accordingly; every consumer of RANGE_UNTIL (if any) is updated.
- **Parser entry point.** A new internal entry point runs a whole parse from any token source (the existing `TokenSourceNext` function pointer and its state): it initialises the parser state, creates the PROGRAM root in `ctx->ast` before parsing, calls the start nonterminal function, checks the end of input and cleans up. The public `parser_parse(ctx, lexer)` becomes a thin wrapper that passes the lexer adapter. This entry point is the test seam.
- **Nonterminal functions.** One per non-expression nonterminal of the LL table, following the template already in the parser module: switch on the current token over the predict sets, create each node and attach it to its parent immediately, copy token text before consuming the token, and report anything else with `parser_syntax_error` listing the expected terminals. Nonterminals that only add structure to the grammar (e.g. tails and lists) attach their results to the parent they were given.
- **Expression nonterminals.** EXPR, OR_TAIL, AND_EXPR, AND_TAIL, EQ_EXPR, EQ_TAIL, REL_EXPR, REL_TAIL, ADD_EXPR, ADD_TAIL, MUL_EXPR, MUL_TAIL, the four `*_OP` nonterminals, UNARY, MEMBER_TAIL, OPERAND, ID_TAIL, NO_IDENT_OPERAND, LITERAL, EXPR_TAIL, BINARY_TAIL and the INTERP_* nonterminals have no nonterminal functions (ADR-0003). Wherever a rule starts with one of them, the nonterminal function calls `expr_parse` with the current token still unconsumed and attaches the returned subtree.
- **Identifier-started statements.** STATEMENT_ID and INLINE_ID are entered with the IDENT still current. They peek: on ASSIGN they consume the IDENT and `=` and parse RVALUE (STATEMENT_ID) or an expression (INLINE_ID) into an ASSIGN node; otherwise they call `expr_parse`, which starts at the IDENT. `parser_peek` is implemented so that the peeked token goes through the token layer (WHITESPACE dropped, raw indents translated); `parser_advance` already moves to a peeked token.
- **Operands.** The precedence parser treats one whole operand as `i`: an IDENT (followed by a call when the next token is `(` or the unit token), a literal, the unit value `()` as UNIT_LIT, or an interpolated string. It builds CALL and INTERP_STRING nodes itself and fills them through callbacks.
- **Callbacks.** The precedence parser calls back into exactly three nonterminal functions, with the same signature as every nonterminal function (they attach to a parent): ARG_LIST with the CALL node as parent, INLINE_VALUE with the INTERP_STRING node as parent (for `${…}`), and IF_INLINE for `(` followed by `if`. For IF_INLINE the parent is a NONE holder node on the precedence stack; afterwards the IF is detached from the holder and becomes the operand, the holder is destroyed. The callback prototypes live in the private parser header.
- **Expression end.** `expr_parse` acts as if it read `$` when the current token is not a precedence terminal; when it is `)` and no `(` of this expression is open; or when it could start an operand (IDENT, a literal, `(`, `!`, an interpolated string start) right after a complete operand or `)` (except `(` or the unit token directly after an IDENT, which is a call). The end token is left current. Then the stack must reduce to a single E; an empty table cell or a failed reduction is a syntax error at the current token.
- **Operator classes.** Tokens map to precedence terminals: PERIOD IDENT to `.m`; `-` to `pre` unless it follows an operand or `)` (then `add`); `!` always to `pre`; `* /` to `mul`; `+ -` to `add`; `< > <= >=` to `rel`; `== !=` to `eq`; `&&` to `and`; `||` to `or`. Each reduction builds the node of the operator token that was shifted (`pre` with `-` builds NEG, with `!` builds NOT). `( E )` reduces to E with no node.
- **Precedence stack ownership (ADR-0004).** The precedence stack is one array in the parser state. Each `expr_parse` call remembers where its frame starts and uses only the entries above it. Every subtree on the stack is owned by the stack until it is attached; `parser_cleanup` destroys whatever is left, so a THROW anywhere leaks nothing.
- **If expressions.** The parser sets the EXPR flag on IF nodes built by IF_INLINE and on the IF of `RVALUE -> STATEMENT_IF` (right side of `val`, `var`, assignment, `return`). It never sets it elsewhere and never reports a missing `else` by itself.
- **Bodies and blocks.** Single-line bodies of `def`, `if`/`else`, `while` and `for` are wrapped in a BLOCK. `else if` becomes an IF as the else child. A statement after `;` is appended to the BLOCK the preceding statement is in (for a single-line function body, that body's BLOCK).
- **For loops.** The generator's two or three expressions are parsed with `expr_parse`; the contextual keywords between them are IDENT tokens whose text is checked: `to` builds RANGE_TO, `until` builds RANGE_UNTIL, `by` adds the step; any other word is a syntax error. Guards (inline and multi-line header) are appended after the body.
- **Prolog.** The IDENT in `import ifj26.*` must be exactly `ifj26`, otherwise a syntax error. The prolog produces no node.
- **Positions.** Every node gets the line and column of the token it starts at; operator nodes get the operator token's position.

## Testing Decisions

- **What a good test is.** A test gives the parser a whole program as a token array and checks the resulting AST (or that a syntax error was thrown). It never calls a nonterminal function or `expr_parse` directly and never inspects the parser state or the precedence stack. Tests read like the program they parse.
- **Main seam: tokens in, AST or syntax error out.** Tests use the new internal entry point with the fake token source already in the parser unit tests (a token array replayed with heap copies of the text, including raw indents at the start of every line, as the lexer contract describes). A test-only printer renders the AST as a compact S-expression (node kind, name or value, flags, children), and tests compare that text, e.g. `(PROGRAM (DEF main [MAIN] (IDENT run) NONE (BLOCK …) (PARAM_LIST)))`. Syntax error tests check the thrown exit code 2; all tests run under valgrind, which covers cleanup after errors (ADR-0004).
- **Coverage at the seam.** Every user story above that describes AST shape, precedence, associativity, expression end, the `-` classification, callbacks (arguments, `${…}`, `(if …)`), the holder node, `;` siblings, the EXPR flag, INTERP_STRING, RANGE_UNTIL with a step, contextual keywords, the prolog check and a representative syntax error in each nonterminal function and in the middle of an expression (to exercise cleanup with a non-empty precedence stack).
- **Secondary seam: e2e cases.** Syntax error cases (a `source` and an `exit_code` of 2 per case folder) are added once the lexer emits EOL and INDENT tokens; until then they are blocked and not required by this spec.
- **No other seams.** No unit tests per nonterminal function and no separate tests of `expr_parse`.
- **Prior art.** The parser unit tests (fake token source, expect/accept, INDENT translation, cleanup on error) and the AST unit tests, both using the snow test framework; e2e fixtures under the e2e tests folder.

## Out of Scope

- Changing the grammar, the LL table, the precedence table or the reduction rules.
- Splitting `()` and `Unit` into separate tokens. The lexer's author will add a separate token for the unit value; afterwards a table maker draft replaces TYPE_UNIT with it in FUNC_PARAMS, CALL_ARGS and LITERAL. Until then the parser accepts `def f Unit`, `readInt Unit`, `val u = Unit` and `val x: () = 1` (known gap).
- Semantic analysis (exit codes 3 to 10), including missing `else` on if expressions, inferred return types, and conversions inside interpolated strings (ADR-0001).
- Lexer fixes found while designing: on the lexer branch no EOL or INDENT tokens are emitted and every token's position is 1:1. These belong to the lexer's author.
- Extensions without grammar rules (lists, lambdas, algebraic data types, extension methods, multiple parameter lists).
- Error recovery: the first syntax error ends compilation.

## Further Notes

- Every code block created or modified must start with the AI attribution comment block required by AGENTS.md.
- Glossary terms introduced for this work: operand, callback, expression end, precedence stack, contextual keyword, interpolated string, if expression, unit value.
- Design session notes: `guidance/parser-structure/GLOSSARY.md`, ADR-0003 (expression nonterminals have no functions), ADR-0004 (precedence stack lives in parser state).
