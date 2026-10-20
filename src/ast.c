#include "ast.h"
#include <stddef.h>
#include <stdlib.h>
#include "ctx.h"

struct AstNode {
  AstNodeType type;
  AstNodeValue value;

  AstNode** child_nodes;
  size_t child_size;
  size_t child_capacity;
};

void ast_node_create(Ctx* ctx, AstNode** node) {
  if (node == NULL) {
    THROW(E_INTERNAL_ERROR, "Cannot create AST node: output pointer is NULL");
  }
  *node = TRY_MALLOC(sizeof(**node));
  **node = (AstNode){.type = AST_NODE_NONE,
                     .value = {.int_value = 0},
                     .child_nodes = NULL,
                     .child_size = 0,
                     .child_capacity = 0};
}

// NOLINTNEXTLINE(misc-no-recursion)
void ast_node_destroy(Ctx* ctx, AstNode* node) {
  if (node == NULL) {
    THROW(E_INTERNAL_ERROR, "Cannot destroy AST node: node pointer is NULL");
  }
  if (node->child_nodes != NULL) {
    for (size_t child_idx = 0; child_idx < node->child_size; child_idx++) {
      ast_node_destroy(ctx, node->child_nodes[child_idx]);
    }
  }
  free(node->child_nodes);
  free(node);
  return;
}
