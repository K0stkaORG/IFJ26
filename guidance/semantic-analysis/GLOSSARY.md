# Semantic analysis

How the IFJ26 compiler checks a finished AST for meaning: which declaration every name refers to, where names are visible, and whether the program's values fit together (exit codes 3 to 7).

## Language

### Names and where they are visible

**Scope**:
A region of the program in which a declared name can be referred to. Every BLOCK has one; a function also has a parameter scope around its body and a `for` loop a loop-variable scope around its body. An `if` or `while` has none of its own.
_Avoid_: symtable (that is how a scope is stored), block (a block has a scope, it is not one), environment, frame

**Global scope**:
The outermost scope, holding every function, built-in and user-defined, and every member. It is the only scope that holds functions.
_Avoid_: root scope, global table, top-level scope

**Parameter scope**:
The scope holding a function's parameters, directly inside the global scope; the function body's scope is nested inside it.
_Avoid_: function scope (ambiguous with the body's scope)

**Loop-variable scope**:
The scope holding a `for` loop's variable, in which its guards are checked; the loop body's scope is nested inside it.
_Avoid_: for scope, iterator scope

**Current scope**:
The scope a new declaration goes into while semantic analysis walks the AST.
_Avoid_: active scope, top scope

**Scope owner**:
The AST node a scope belongs to: a BLOCK for its scope, a DEF for its parameter scope, a FOR for its loop-variable scope. The scope lives as long as the node. The global scope has no owner; it is the context's global symtable. Leaving a scope makes its enclosing scope current but keeps it.
_Avoid_: scope registry, scope stack, symtable array

### Declaring and referring

**Declaration**:
The place a name is introduced into a scope: a `def`, a parameter, a `val`, a `var` or a `for` variable. A `val` or `var` is declared after its initializer is checked, so the initializer still sees any outer name it shadows.
_Avoid_: definition (except for functions), binding

**Symbol**:
Everything known about one declared name that semantic analysis needs: whether it is a function or a variable, its type, whether it can be written, its parameters. A parameter cannot be written, like a `val`.
_Avoid_: symtable entry, symtable data, record

**Resolution**:
Finding the symbol a name refers to, searching from the current scope outward. A call resolves only among functions, any other identifier only among variables, so a variable may share a function's name.
_Avoid_: lookup (that is the symtable operation), binding

**Shadowing**:
A declaration in an inner scope hiding a name of an enclosing scope until the inner scope ends.
_Avoid_: hiding, overriding

**Redefinition**:
Declaring a name that is already declared in the same scope, exit code 4. Declaring a user function with a built-in's name is one too.
_Avoid_: duplicate declaration, redeclaration

**Function collection**:
The first phase of semantic analysis, which declares every function in the global scope before any function body is checked, so a function can be called before its definition.
_Avoid_: pre-pass, forward declaration

**Slot**:
The number a variable gets within its function, stored in its symbol and shown on every use of the variable. The parameters take 0 to n-1, then every `val`, `var` and `for` variable takes the next number in the order of its declaration, so a shadowed name has a slot of its own. A slot is not an address; IR generation maps it to storage.
_Avoid_: index, offset, register, local number

**Function id**:
The number of a user function: the position of its `def` among the program's top-level `def`s, assigned by function collection and stored in the function's symbol. It finds the `def` from a call, and tells a user function from a built-in or member, whose symbols carry no function id of this kind.
_Avoid_: function index, function number, address

### Types

**Widening**:
The implicit conversion of an Int value to Double where a Double is required: in an initializer, an assignment, an argument, a `return` or mixed arithmetic. Semantic analysis makes every widening explicit in the AST. Nothing narrows implicitly.
_Avoid_: promotion, coercion, cast

**Inferred return type**:
The return type of a function whose `def` omits it, taken from the type of its body's last statement. Inferring it through a call to the function itself is an error.
_Avoid_: implicit return type, deduced type

**Body result**:
The value a function body yields: the value of its last statement, or, when that is an `if`, of the last statement of each of its branches, recursively. It is compared with the declared return type, or, for an omitted one, becomes the return type. A branch ending in `return` yields no body result and is compatible with the other branch; a declared `Unit` discards every body result. The branches of an if expression follow the same rule with no declared type. In the code it is a result rule.
_Avoid_: tail, last value, final expression

**On-demand inference**:
Checking a function whose return type is omitted at the moment a call needs its type, instead of waiting for its place in source order, so a call before the `def` gets the right type. The function is marked as being checked meanwhile; a call that reaches it again is a cycle through a function without a declared return type (exit 6). A cycle that passes through a function with a declared type stops inference there, so a final phase searches the calls for it instead; every function in a cycle must declare its type (ADR-0005).
_Avoid_: lazy inference, forward inference, demand-driven checking

**Member**:
A built-in operation written after a dot on a value (`x.toInt`, `s.length`, `(-3.75).toInt.toString`). Which members exist depends on the receiver's type; they live in the global scope beside the built-in functions but can never clash with a user's name.
_Avoid_: method, property, field
