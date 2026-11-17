# Every function in a call cycle declares its return type

A function with an omitted return type that is part of any call cycle, direct or indirect, stops with exit 6, even when another function in the cycle declares its type. So `def f() = g()` with `def g(): Int = f()` is an error, in either source order. The PDF specification (section 2.3) requires an explicit return type "if direct or indirect recursion occurs", and the checks apply to unreachable code too, so a call after a `return` still closes a cycle.

On-demand inference only notices a cycle while it is inferring a function in it, and a declared function in the cycle stops the inference. So a separate last phase (`check_recursion`) searches the call graph from every function without a declared return type for a path back to itself. The search marks visited DEFs with the existing `AST_FLAG_DEF_CHECKING` flag, which is free once every DEF is checked, so the pass still allocates nothing.

This reverses the choice in the semantic analysis spec ("Ambiguities decided here", story 76), which followed Scala and let a declared type anywhere in the cycle break it.

## Considered Options

- **Scala's behaviour, as the internal spec had it.** Rejected: the PDF asks for the declared type on the recursive function itself.
- **Detecting the cycle during inference.** Rejected: once a declared function is reached inference stops, so the cycle stays hidden.
