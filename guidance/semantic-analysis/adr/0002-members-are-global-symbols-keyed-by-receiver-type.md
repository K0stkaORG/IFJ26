# Members are global symbols keyed by receiver type

Every member (`x.toInt`, `s.length`, `v.toString`) is a function symbol in the global scope, stored under the key `"<ReceiverType>.<member>"` (`"Double.toInt"`, `"String.length"`, `"Int.toString"`), with the receiver as its only parameter, its result type as the symbol's type and its own IR function type. A member that works on several receiver types has one entry per type. Semantic analysis resolves `receiver.name` by building the key from the receiver's type, and the MEMBER node points at the symbol like any other resolved name, so its type and its IR come from one place, just as for a call. A `.` cannot appear in an identifier, so no key can clash with a user's function or variable.

Error codes follow from the keys: when no key exists for the receiver's type but the member name exists on another type (`"abc".toInt`), it is a type error (exit 6). When the name exists on no type (`x.foo`), the member is undefined (exit 3).

## Considered Options

- **Members under their plain names (`toString`, `length`).** Rejected: the global scope also holds user functions, so a user's `def length(…)` would become a redefinition (exit 4) of a name the user never declared, and one name could not have a different type on each receiver.
- **Members mapped onto existing built-in functions by a fixed table in the pass (`.toInt` to `toInt`, `.toString` to `toStr`, `.length` to `strLength`), checked as a one-argument call.** Rejected: members without a built-in function (`.floor`) would still need special handling, and member facts would live in a table in the pass instead of in the symtable, against the rule that everything semantic analysis knows about a name lives in its symbol.

## Amendment: who inserts the members

The built-ins file is not changed (only the AST, the parser and semantic analysis may change, see ADR-0003). Semantic analysis inserts the member entries into the global scope itself, through the symtable API, before function collection. VALMEM allows exactly four members, so there are eight entries: `Int.toString`, `Double.toString`, `Boolean.toString`, `String.toString`, `Unit.toString` (IR function `toStr`), `Double.toInt` (`toInt`), `String.length` (`strLength`) and `Double.floor`. Like the built-ins, a member's `ir_index` holds its IR function type. `Double.floor` has no IR function (the IR has a floor instruction instead), so its `ir_index` is unused and IR generation recognises it by the MEMBER node's name.
