/**
 * @file ast.h
 * Project: IFJ26
 *
 * @author Lukáš Pollák (xpollal00)
 * @date 31. 10. 2026
 *
 * @brief Unit test for the AST
 */

#define SNOW_ENABLED 1
#include "snow/snow.h"
#include "ast.h"

describe(AST_NODE) {
  subdesc(CREATE) {
    it("Should create a valid AST node") {
      AstNode* node = NULL;

      ast_node_create(ctx, &node);
      defer({
        if (node != NULL) {
          ast_node_destroy(ctx, node);
        }
      });

      assertneq_ptr(node, NULL);
      asserteq(ast_node_get_type(node), AST_NODE_NONE);
      asserteq(ast_node_get_child_count(node), 0);
      asserteq(ast_node_get_value(node).int_value, 0);

      AstNode* parent = node;
      ast_node_get_parent(node, &parent);
      asserteq_ptr(parent, NULL);
    }
    
    it("Should throw an error when node address is NULL") {
      assertThrow(E_INTERNAL_ERROR, {
        ast_node_create(ctx, NULL);
      });
    }

    it("Should throw an error when allocation fails") {
      static AstNode* node;
      node = NULL;
      defer({
        if (node != NULL) {
          ast_node_destroy(ctx, node);
        }
      });

      ctx->flags.forceNthMallocFail = 1;

      assertThrow(E_INTERNAL_ERROR, {
        ast_node_create(ctx, &node);
      });
    }
  }
  
  subdesc(DESTROY) {
    it("Should destroy a node without children") {
      static AstNode* node;
      node = NULL;
      defer({
        if (node != NULL) {
          ast_node_destroy(ctx, node);
        }
      });

      ast_node_create(ctx, &node);

      assertNoThrow({
        ast_node_destroy(ctx, node);
        node = NULL;
      });
    }

    it("Should destroy a node and its descendants") {
      static AstNode* root;
      static AstNode* pending;
      root = NULL;
      pending = NULL;
      defer({
        if (pending != NULL) {
          ast_node_destroy(ctx, pending);
        }
        if (root != NULL) {
          ast_node_destroy(ctx, root);
        }
      });

      ast_node_create(ctx, &root);
      for (size_t child_index = 0; child_index < 4; child_index++) {
        ast_node_create(ctx, &pending);
        ast_node_append_child(ctx, root, pending);
        pending = NULL;
      }

      AstNode* first_child = NULL;
      ast_node_get_child(ctx, root, 0, &first_child);
      ast_node_create(ctx, &pending);
      ast_node_append_child(ctx, first_child, pending);
      pending = NULL;

      assertNoThrow({
        ast_node_destroy(ctx, root);
        root = NULL;
      });
    }

    it("Should throw an error when node is NULL") {
      assertThrow(E_INTERNAL_ERROR, {
        ast_node_destroy(ctx, NULL);
      });
    }
  }
}

snow_main();
