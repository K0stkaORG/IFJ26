# Scopes are kept in a registry until compilation ends, not popped

Superseded by ADR-0003 (scopes are owned by the AST nodes that open them): the registry needed a new field in the shared context, which this work may not change.

Semantic analysis keeps every scope it creates in a scope registry in the shared context (a dynamic array of heap-allocated symtables, the global scope first) and frees them only in `ctx_free`. Leaving a block moves the current scope to its parent; nothing is popped. Everything semantic analysis knows about a name lives in that name's symbol, and AST identifiers point at their symbol, so later passes (IR generation) read a variable's type, writability and slot from the same place semantic analysis wrote it, and tell two shadowed `x` apart by their symbol pointers.

The global symtable stops being a separate `Ctx` field: it is the registry's first scope, owned and freed like every other.

## Considered Options

- **A scope stack, popped and destroyed when its block ends.** Rejected: popping frees the block's symbols, so the AST `symbol` pointer of every local variable would dangle. Later passes would need the facts copied onto the AST before each pop, plus a new per-variable id field (or renamed identifiers) to tell shadowed variables apart. It would only save memory, and an IFJ26 program's scopes take a few kilobytes.
- **Symtables stored by value in the array.** Rejected: `Symtable.parent` is a pointer, and growing the array moves every table, so all parent links would dangle.
