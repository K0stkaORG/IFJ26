# The parser owns every token the lexer returns

Once the lexer's next-token function returns a token, the parser owns it, including `value.text`, and frees it when it moves past the token. The lexer keeps no references to tokens it has handed out. The AST never points at token text, because `ast_node_set_name` and `ast_node_set_value` store their own copies.

This is a cross-module contract with the lexer, which a different team member writes, so it must not be changed on one side only. The exact signature of the next-token function is the lexer owner's choice.
