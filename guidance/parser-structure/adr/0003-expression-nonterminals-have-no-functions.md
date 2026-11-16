# Expression nonterminals of the LL table have no nonterminal functions

The LL table spells out expressions as LL nonterminals (`EXPR`, `AND_EXPR` … `UNARY`, `MEMBER_TAIL`, `OPERAND`, `ID_TAIL`, `NO_IDENT_OPERAND`, `LITERAL`, `EXPR_TAIL`, `BINARY_TAIL`, the `*_OP` nonterminals). The code implements none of them: wherever one appears in a rule, the recursive descent calls the precedence parser. The rules stay in the table as the specification of what an expression is; the precedence table and reduction rules are what the code runs.

The precedence parser treats a whole operand (identifier, call, literal, `()`, interpolated string) as one `i`. To build the parts of an operand that contain full values, it calls back into exactly three nonterminal functions: `ARG_LIST` (call arguments), `INLINE_VALUE` (`${…}`) and `IF_INLINE` (`(` followed by `if`).

## Considered Options

- **Implement the expression nonterminals as recursive descent too.** Rejected: two parsers for the same language, and the precedence parser is required by the assignment.
