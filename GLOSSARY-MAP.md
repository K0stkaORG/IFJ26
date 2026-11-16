# Glossary Map

Design notes live in `guidance/<topic>/`, one folder per part of the compiler a design session addressed. Each folder holds that topic's `GLOSSARY.md` and its ADRs in `adr/` (numbered `0001-slug.md`, per topic). A new topic gets a new folder and a new row below.

## Contexts

- [Parser structure](./guidance/parser-structure/GLOSSARY.md): how the parser turns tokens into an AST: the LL-table recursive descent, the precedence parser for expressions, the token layer (lookahead, indentation into blocks) and syntax errors. ADRs: [`guidance/parser-structure/adr/`](./guidance/parser-structure/adr/)
- [Semantic analysis](./guidance/semantic-analysis/GLOSSARY.md): the pass after the parser that checks the AST for meaning: scopes, shadowing, resolving names to symbols, and type and argument checks (exit codes 3 to 7). ADRs: [`guidance/semantic-analysis/adr/`](./guidance/semantic-analysis/adr/)
