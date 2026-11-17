# Parser structure

How the IFJ26 compiler turns the lexer's token stream into a verified program: the recursive descent driven by the LL table, the token lookahead it reads, and how it reports errors.

## Language

### Driving the parse

**LL table**:
The hand-made table, owned by the parser author, that picks a production for each pair of a nonterminal and the current token. It is the single source of truth for the grammar.
_Avoid_: parse table, grammar table

**Nonterminal function**:
The parse routine for exactly one nonterminal of the LL table; its alternatives are the table's rows for that nonterminal. The expression nonterminals (`EXPR` down to `UNARY`, `OPERAND`, `LITERAL`, `EXPR_TAIL`, `BINARY_TAIL` and the operator nonterminals) have none: the precedence parser stands in for them.
_Avoid_: rule function, parse method

**Precedence parser**:
The bottom-up parser that the recursive descent hands every expression to; it builds and returns that expression's AST subtree.
_Avoid_: expression parser (ambiguous), operator parser

**Operand**:
One `i` of the precedence table: an identifier, a call, a literal, `()` or an interpolated string.
_Avoid_: atom, primary, term

**Expression end**:
The token at which the precedence parser stops and acts as if it read `$`: a token that is not a precedence terminal, a `)` with no `(` of this expression open, or a token that could start an operand right after a complete operand or `)` (except `(` after an identifier, which is a call). The token is left as the current token.
_Avoid_: terminator, delimiter

**Precedence stack**:
The precedence parser's stack of terminals and partly built subtrees. One stack is shared by all expressions being parsed at once (nested through callbacks), each using the part above where it started.
_Avoid_: expression stack, operator stack

**Callback**:
A nonterminal function the precedence parser calls to build part of an operand: `ARG_LIST` for call arguments, `INLINE_VALUE` inside `${…}`, `IF_INLINE` after `(`.
_Avoid_: re-entry, nested parse

**Predict set**:
The terminals for which the LL table chooses a given production of a nonterminal.
_Avoid_: first set (that is only part of it), lookahead set

**Syntax analysis**:
Checking that the program's tokens form a program the grammar accepts, and building its AST. It is everything the parser does.
_Avoid_: verification, parsing check

**Semantic analysis**:
Checking a complete AST for meaning: undefined or redefined names, argument counts, types, writes to `val`. It is a separate pass after the parser and is not part of the parser.
_Avoid_: semantic actions, verification

### Program shapes

**Interpolated string**:
A string literal prefixed with `s` (`s"…"` or `s"""…"""`) whose `$name` and `${…}` parts are values turned into text.
_Avoid_: s-string, template string, string concatenation

**If expression**:
An `if` written where a value is required: inside parentheses, a call argument or `${…}`, or as the right side of `val`, `var`, an assignment or `return`. Its `else` is mandatory. An `if` that merely ends a body is not one by its position; whether its value is used is decided by semantic analysis.
_Avoid_: ternary, inline if

**Unit value**:
`()`, the only value of the type `Unit`. It also stands for an empty parameter or argument list (`def f()`, `readInt()`). It is a different token from the type name `Unit`.
_Avoid_: unit type (that is `Unit`), empty parens

### Reading tokens

**Current token**:
The token the parser is deciding on now; the LL table is indexed by it.
_Avoid_: lookahead token, cur

**Peek**:
Looking at the token after the current token without consuming anything, used only where one token of lookahead is not enough: a statement starting with an identifier peeks for `=` to tell an assignment from an expression, so the precedence parser can start at the identifier.
_Avoid_: lookahead(2), unget

**Indentation**:
The number of leading spaces of a source line, carried by an INDENT token as its value.
_Avoid_: indent level, depth

**Block open / Block close**:
The two roles of the INDENT terminal in the LL table (`BLOCK -> INDENT STATEMENT STATEMENT_LIST INDENT`). The first INDENT means indentation increased; the last means indentation returned to the enclosing level.
_Avoid_: DEDENT, scope start/end

**Contextual keyword**:
An IDENT token whose text the grammar requires to be one specific word: `to`, `until` and `by` in a `for` generator, `ifj26` in the prolog. Any other word there is a syntax error.
_Avoid_: soft keyword, reserved identifier

**Raw indent**:
The INDENT token as the lexer emits it at the start of an indented line, carrying the line's indentation. A line in column 0 has no raw indent: a line starting with any other token counts as indentation 0. The parser only ever sees block open / block close, which it derives from raw indents.
_Avoid_: indent token (ambiguous)

### Failing

**Syntax error**:
Input the LL table has no production for, reported with exit code 2 through the shared context, ending compilation immediately.
_Avoid_: parse error, parser failure
