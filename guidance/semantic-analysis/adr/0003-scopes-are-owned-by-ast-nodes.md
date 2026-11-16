# Scopes are owned by the AST nodes that open them

Supersedes ADR-0001.

Every scope except the global scope is a heap-allocated symtable owned by the AST node that opens it: a BLOCK owns its scope, a DEF its parameter scope, a FOR its loop-variable scope. The AST module creates the scope and attaches it to the node in one step, and destroying the node destroys its scope. The global scope is the context's existing global symtable, which already holds the built-in functions; semantic analysis adds the members and user functions to it through the symtable API. Leaving a scope makes its parent current; nothing is destroyed during semantic analysis.

So scopes and their symbols live exactly as long as the AST. AST identifiers point at their symbol, so IR generation reads a variable's type, writability and slot from the same place semantic analysis wrote it, and it tells two shadowed `x` apart by their symbol pointers. A THROW anywhere in the pass leaks nothing, because the context already frees the AST (and with it every scope) before it destroys the global symtable.

Only the AST, the parser and semantic analysis may change. The shared context, the symtable, the built-ins and the IR are someone else's code and stay untouched. That is why the registry from ADR-0001, which needed a new field in the context, was dropped.

## Considered Options

- **A scope registry in the context (ADR-0001).** Rejected: it adds a field to the shared context and changes how the context frees memory, which is outside the modules this work may change.
- **A registry owned by the semantic analysis module, freed by the pass itself.** Rejected: on a THROW the context's cleanup does not know about it, so the pass would need its own error trap around the whole walk, and scopes would still have to outlive the pass for IR generation.
- **A scope stack, popped and destroyed when its block ends.** Rejected for the same reason as in ADR-0001: popping frees the block's symbols, and every AST `symbol` pointer to a local variable would dangle.
