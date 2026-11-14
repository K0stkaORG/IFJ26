# Parser structure

How the IFJ26 compiler turns the lexer's token stream into a verified program: the recursive descent driven by the LL table, the token lookahead it reads, and how it reports errors.

## Language

### Driving the parse

**LL table**:
The hand-made table, owned by the parser author, that picks a production for each pair of a nonterminal and the current token. It is the single source of truth for the grammar.
_Avoid_: parse table, grammar table

**Nonterminal function**:
The parse routine for exactly one nonterminal of the LL table; its alternatives are the table's rows for that nonterminal.
_Avoid_: rule function, parse method

**Precedence parser**:
The bottom-up parser that the recursive descent hands every expression to; it builds and returns that expression's AST subtree.
_Avoid_: expression parser (ambiguous), operator parser

**Predict set**:
The terminals for which the LL table chooses a given production of a nonterminal.
_Avoid_: first set (that is only part of it), lookahead set

**Syntax analysis**:
Checking that the program's tokens form a program the grammar accepts, and building its AST. It is everything the parser does.
_Avoid_: verification, parsing check

**Semantic analysis**:
Checking a complete AST for meaning: undefined or redefined names, argument counts, types, writes to `val`. It is a separate pass after the parser and is not part of the parser.
_Avoid_: semantic actions, verification

### Reading tokens

**Current token**:
The token the parser is deciding on now; the LL table is indexed by it.
_Avoid_: lookahead token, cur

**Peek**:
Looking at the token after the current token without consuming anything, used only where one token of lookahead is not enough.
_Avoid_: lookahead(2), unget

**Indentation**:
The number of leading spaces of a source line, carried by an INDENT token as its value.
_Avoid_: indent level, depth

**Block open / Block close**:
The two roles of the INDENT terminal in the LL table (`BLOCK -> INDENT STATEMENT STATEMENT_LIST INDENT`). The first INDENT means indentation increased; the last means indentation returned to the enclosing level.
_Avoid_: DEDENT, scope start/end

**Raw indent**:
The INDENT token as the lexer emits it, carrying the line's indentation. The parser only ever sees block open / block close, which it derives from raw indents.
_Avoid_: indent token (ambiguous)

### Failing

**Syntax error**:
Input the LL table has no production for, reported with exit code 2 through the shared context, ending compilation immediately.
_Avoid_: parse error, parser failure
