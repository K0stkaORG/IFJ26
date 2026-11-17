# An undeclared assignment target is exit 7

Assigning to a name that is no visible variable (`x = 1` with no `x` in scope, a variable of another function, or a function name such as `print`) stops with exit 7, the same as assigning to a `val`, a parameter or a `for` variable. The PDF specification (section 2.2) says the left side "must denote a previously defined modifiable variable (otherwise error 7)", and section 6 says the more specific code wins when more than one fits. The target is checked before the value, so `x = y` with neither defined is exit 7, not 3. Using an undefined name anywhere else is still exit 3.

This reverses the choice in the semantic analysis spec (story 40 and "Ambiguities decided here"), which picked 3 because "undefined" seemed the more specific category.

## Considered Options

- **Exit 3, as the internal spec had it.** Rejected: the PDF names 7 for this exact statement, so 7 is the more specific code.
