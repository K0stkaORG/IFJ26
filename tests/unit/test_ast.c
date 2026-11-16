/**
 * @file ast.h
 * Project: IFJ26
 *
 * @author Lukáš Pollák (xpollal00)
 * @date 31. 10. 2026
 *
 * @brief Unit test for the AST
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "helpers/ast_test_string.h"
#include "snow/snow.h"

#define AST_TEST_NODE_CAPACITY 32

static AstNode* ast_test_nodes[AST_TEST_NODE_CAPACITY];
static size_t ast_test_node_count;

static AstNode* ast_test_node(Ctx* ctx) {
  assert(ast_test_node_count <
         sizeof(ast_test_nodes) / sizeof(ast_test_nodes[0]));
  AstNode* node = NULL;
  ast_node_create(ctx, &node);
  ast_test_nodes[ast_test_node_count++] = node;
  return node;
}

static void ast_test_attach_child(Ctx* ctx,
                                  AstNode* parent,
                                  AstNode* child,
                                  bool insert) {
  if (insert) {
    ast_node_insert_child(ctx, parent, 0, child);
  } else {
    ast_node_append_child(ctx, parent, child);
  }
}

describe(AST_NODE) {
  subdesc(CREATE) {
    it("Should create a valid AST node") {
      // Arrange: prepare the output pointer for the new node.
      AstNode* node = NULL;

      // Act: create the node and register cleanup.
      ast_node_create(ctx, &node);
      defer({
        if (node != NULL) {
          ast_node_destroy(ctx, node);
        }
      });

      // Assert: check the initial state exposed by the public API.
      assertneq_ptr(node, NULL);
      asserteq(ast_node_get_type(node), AST_NODE_NONE);
      asserteq(ast_node_get_child_count(node), 0);
      asserteq(ast_node_get_value(node).int_value, 0);

      AstNode* parent = node;
      parent = ast_node_get_parent(node);
      asserteq_ptr(parent, NULL);
    }
  }

  subdesc(DESTROY) {
    it("Should destroy a node and its children") {
      // Arrange: build a tree with multiple children and a grandchild.
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

      // Act: destroy the root and its owned descendants.
      assertNoThrow({
        ast_node_destroy(ctx, root);
        root = NULL;
      });

      // Assert: check that recursive destruction completed without an error.
      asserteq(ctx->exit_code, E_OK);
    }

    it("Should compact the parent when destroying any attached subtree") {
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

      for (size_t child_count = 1; child_count <= 4; child_count++) {
        for (size_t removed_index = 0; removed_index < child_count;
             removed_index++) {
          AstNode* children[4] = {NULL};
          ast_node_create(ctx, &root);
          for (size_t child_index = 0; child_index < child_count;
               child_index++) {
            ast_node_create(ctx, &pending);
            ast_node_append_child(ctx, root, pending);
            children[child_index] = pending;
            pending = NULL;
          }
          for (size_t descendant_index = 0; descendant_index < 3;
               descendant_index++) {
            ast_node_create(ctx, &pending);
            ast_node_append_child(ctx, children[removed_index], pending);
            pending = NULL;
          }

          assertNoThrow({ ast_node_destroy(ctx, children[removed_index]); });
          children[removed_index] = NULL;

          asserteq(ast_node_get_child_count(root), child_count - 1);
          size_t remaining_index = 0;
          for (size_t child_index = 0; child_index < child_count;
               child_index++) {
            if (child_index == removed_index) {
              continue;
            }
            AstNode* actual = NULL;
            ast_node_get_child(ctx, root, remaining_index++, &actual);
            asserteq_ptr(actual, children[child_index]);
            actual = ast_node_get_parent(actual);
            asserteq_ptr(actual, root);
          }
          AstNode* actual = NULL;
          assertThrow(E_INTERNAL_ERROR, {
            ast_node_get_child(ctx, root, child_count - 1, &actual);
          });
          ctx->exit_code = E_OK;

          ast_node_create(ctx, &pending);
          ast_node_append_child(ctx, root, pending);
          AstNode* replacement = pending;
          pending = NULL;
          asserteq(ast_node_get_child_count(root), child_count);
          ast_node_get_child(ctx, root, child_count - 1, &actual);
          asserteq_ptr(actual, replacement);
          actual = ast_node_get_parent(replacement);
          asserteq_ptr(actual, root);

          assertNoThrow({
            ast_node_destroy(ctx, root);
            root = NULL;
          });
        }
      }
    }
  }
}

describe(AST_NODE_OPERATIONS) {
  before_each() {
    ast_test_node_count = 0;
  }

  after_each() {
    ctx->flags.forceNthMallocFail = 0;
    for (size_t index = 0; index < ast_test_node_count; index++) {
      while (ast_node_get_child_count(ast_test_nodes[index]) > 0) {
        AstNode* detached = NULL;
        ast_node_detach_child(ctx, ast_test_nodes[index], 0, &detached);
      }
    }
    for (size_t index = 0; index < ast_test_node_count; index++) {
      ast_node_destroy(ctx, ast_test_nodes[index]);
    }
    ast_test_node_count = 0;
  }

  subdesc(ATTACHMENT_REGRESSIONS) {
    it("Should reject duplicate children in append and insert") {
      for (size_t operation = 0; operation < 2; operation++) {
        // Arrange: attach two children to expose unintended insertion shifts.
        ctx->exit_code = E_OK;
        AstNode* parent = ast_test_node(ctx);
        AstNode* first = ast_test_node(ctx);
        AstNode* second = ast_test_node(ctx);
        ast_node_append_child(ctx, parent, first);
        ast_node_append_child(ctx, parent, second);

        // Act: try to attach the second child again.
        assertThrow(E_INTERNAL_ERROR, {
          ast_test_attach_child(ctx, parent, second, operation == 1);
        });

        // Assert: preserve both children in order and their original parent.
        asserteq(ast_node_get_child_count(parent), 2);
        AstNode* actual = NULL;
        ast_node_get_child(ctx, parent, 0, &actual);
        asserteq_ptr(actual, first);
        ast_node_get_child(ctx, parent, 1, &actual);
        asserteq_ptr(actual, second);
        actual = ast_node_get_parent(first);
        asserteq_ptr(actual, parent);
        actual = ast_node_get_parent(second);
        asserteq_ptr(actual, parent);
      }
    }

    it("Should reject children owned by another parent in append and insert") {
      for (size_t operation = 0; operation < 2; operation++) {
        // Arrange: give both the source and destination an existing child.
        ctx->exit_code = E_OK;
        AstNode* source = ast_test_node(ctx);
        AstNode* destination = ast_test_node(ctx);
        AstNode* child = ast_test_node(ctx);
        AstNode* existing = ast_test_node(ctx);
        ast_node_append_child(ctx, source, child);
        ast_node_append_child(ctx, destination, existing);

        // Act: try to attach the source's child to the destination.
        assertThrow(E_INTERNAL_ERROR, {
          ast_test_attach_child(ctx, destination, child, operation == 1);
        });

        // Assert: preserve each parent's child and both ownership pointers.
        asserteq(ast_node_get_child_count(source), 1);
        asserteq(ast_node_get_child_count(destination), 1);
        AstNode* actual = NULL;
        ast_node_get_child(ctx, source, 0, &actual);
        asserteq_ptr(actual, child);
        ast_node_get_child(ctx, destination, 0, &actual);
        asserteq_ptr(actual, existing);
        actual = ast_node_get_parent(child);
        asserteq_ptr(actual, source);
        actual = ast_node_get_parent(existing);
        asserteq_ptr(actual, destination);
      }
    }

    it("Should reject self-attachment in append and insert") {
      for (size_t operation = 0; operation < 2; operation++) {
        // Arrange: create a detached node with no children.
        ctx->exit_code = E_OK;
        AstNode* node = ast_test_node(ctx);

        // Act: try to make the node its own child.
        assertThrow(E_INTERNAL_ERROR, {
          ast_test_attach_child(ctx, node, node, operation == 1);
        });

        // Assert: leave the node detached and childless.
        asserteq(ast_node_get_child_count(node), 0);
        AstNode* actual = node;
        actual = ast_node_get_parent(node);
        asserteq_ptr(actual, NULL);
      }
    }

    it("Should reject root attachment beneath descendants in append and "
       "insert") {
      for (size_t operation = 0; operation < 2; operation++) {
        // Arrange: build a three-level tree whose root has no parent.
        ctx->exit_code = E_OK;
        AstNode* root = ast_test_node(ctx);
        AstNode* middle = ast_test_node(ctx);
        AstNode* leaf = ast_test_node(ctx);
        ast_node_append_child(ctx, root, middle);
        ast_node_append_child(ctx, middle, leaf);

        // Act: try to close a cycle at each descendant level.
        assertThrow(E_INTERNAL_ERROR, {
          ast_test_attach_child(ctx, middle, root, operation == 1);
        });
        assertThrow(E_INTERNAL_ERROR, {
          ast_test_attach_child(ctx, leaf, root, operation == 1);
        });

        // Assert: preserve the complete acyclic tree and its parent links.
        asserteq(ast_node_get_child_count(root), 1);
        asserteq(ast_node_get_child_count(middle), 1);
        asserteq(ast_node_get_child_count(leaf), 0);
        AstNode* actual = NULL;
        ast_node_get_child(ctx, root, 0, &actual);
        asserteq_ptr(actual, middle);
        ast_node_get_child(ctx, middle, 0, &actual);
        asserteq_ptr(actual, leaf);
        actual = ast_node_get_parent(root);
        asserteq_ptr(actual, NULL);
        actual = ast_node_get_parent(middle);
        asserteq_ptr(actual, root);
        actual = ast_node_get_parent(leaf);
        asserteq_ptr(actual, middle);
      }
    }
  }

  subdesc(ATTACHMENT_TRANSFERS) {
    it("Should allow detached children to move through append and insert") {
      for (size_t operation = 0; operation < 2; operation++) {
        // Arrange: detach a child and prepare a destination with one sibling.
        AstNode* source = ast_test_node(ctx);
        AstNode* destination = ast_test_node(ctx);
        AstNode* child = ast_test_node(ctx);
        AstNode* existing = ast_test_node(ctx);
        ast_node_append_child(ctx, source, child);
        ast_node_append_child(ctx, destination, existing);
        AstNode* detached = NULL;
        ast_node_detach_child(ctx, source, 0, &detached);

        // Act: transfer ownership of the detached child to the destination.
        assertNoThrow({
          ast_test_attach_child(ctx, destination, detached, operation == 1);
        });

        // Assert: preserve the sibling and expose the requested child order.
        asserteq_ptr(detached, child);
        asserteq(ast_node_get_child_count(source), 0);
        asserteq(ast_node_get_child_count(destination), 2);
        AstNode* actual = NULL;
        ast_node_get_child(ctx, destination, operation == 1 ? 0 : 1, &actual);
        asserteq_ptr(actual, child);
        ast_node_get_child(ctx, destination, operation == 1 ? 1 : 0, &actual);
        asserteq_ptr(actual, existing);
        actual = ast_node_get_parent(child);
        asserteq_ptr(actual, destination);
        actual = ast_node_get_parent(existing);
        asserteq_ptr(actual, destination);
      }
    }

    it("Should allow independent subtrees in append and insert") {
      for (size_t operation = 0; operation < 2; operation++) {
        // Arrange: build an independent subtree and a destination sibling.
        AstNode* parent = ast_test_node(ctx);
        AstNode* existing = ast_test_node(ctx);
        AstNode* subtree = ast_test_node(ctx);
        AstNode* leaf = ast_test_node(ctx);
        ast_node_append_child(ctx, parent, existing);
        ast_node_append_child(ctx, subtree, leaf);

        // Act: attach the parentless subtree without disturbing descendants.
        assertNoThrow(
            { ast_test_attach_child(ctx, parent, subtree, operation == 1); });

        // Assert: preserve sibling order and every new or existing parent link.
        asserteq(ast_node_get_child_count(parent), 2);
        asserteq(ast_node_get_child_count(subtree), 1);
        asserteq(ast_node_get_child_count(leaf), 0);
        AstNode* actual = NULL;
        ast_node_get_child(ctx, parent, operation == 1 ? 0 : 1, &actual);
        asserteq_ptr(actual, subtree);
        ast_node_get_child(ctx, parent, operation == 1 ? 1 : 0, &actual);
        asserteq_ptr(actual, existing);
        ast_node_get_child(ctx, subtree, 0, &actual);
        asserteq_ptr(actual, leaf);
        actual = ast_node_get_parent(subtree);
        asserteq_ptr(actual, parent);
        actual = ast_node_get_parent(leaf);
        asserteq_ptr(actual, subtree);
        actual = ast_node_get_parent(existing);
        asserteq_ptr(actual, parent);
        actual = ast_node_get_parent(parent);
        asserteq_ptr(actual, NULL);
      }
    }
  }

  subdesc(GET_CHILD) {
    it("Should reject child indices at the count and SIZE_MAX") {
      // Arrange: prepare resources for the function.
      AstNode* parent = ast_test_node(ctx);
      AstNode* child = ast_test_node(ctx);
      ast_node_append_child(ctx, parent, child);
      size_t indices[] = {1, SIZE_MAX};

      // Act: execute the function.
      for (size_t index = 0; index < 2; index++) {
        AstNode* actual = NULL;
        assertThrow(E_INTERNAL_ERROR, {
          ast_node_get_child(ctx, parent, indices[index], &actual);
        });
      }

      // Assert: check the observable result.
      asserteq(ctx->exit_code, E_INTERNAL_ERROR);
      asserteq(ast_node_get_child_count(parent), 1);
    }
  }

  subdesc(ATTACHMENT_ALLOCATION) {
    it("Should preserve a full buffer after failed growth and allow retry") {
      for (size_t operation = 0; operation < 2; operation++) {
        AstNode* parent = ast_test_node(ctx);
        AstNode* existing[3];
        for (size_t index = 0; index < 3; index++) {
          existing[index] = ast_test_node(ctx);
          ast_node_append_child(ctx, parent, existing[index]);
        }
        AstNode* child = ast_test_node(ctx);

        for (size_t attempt = 0; attempt < 2; attempt++) {
          ctx->exit_code = E_OK;
          ctx->flags.forceNthMallocFail = 1;
          assertThrow(E_INTERNAL_ERROR, {
            if (operation == 0) {
              ast_node_append_child(ctx, parent, child);
            } else {
              ast_node_insert_child(ctx, parent, 1, child);
            }
          });

          asserteq(ctx->flags.forceNthMallocFail, 0);
          asserteq(ast_node_get_child_count(parent), 3);
          AstNode* actual = NULL;
          actual = ast_node_get_parent(child);
          asserteq_ptr(actual, NULL);
          for (size_t index = 0; index < 3; index++) {
            ast_node_get_child(ctx, parent, index, &actual);
            asserteq_ptr(actual, existing[index]);
            actual = ast_node_get_parent(existing[index]);
            asserteq_ptr(actual, parent);
          }
        }

        ctx->exit_code = E_OK;
        ctx->flags.forceNthMallocFail = 0;
        assertNoThrow({
          if (operation == 0) {
            ast_node_append_child(ctx, parent, child);
          } else {
            ast_node_insert_child(ctx, parent, 1, child);
          }
        });

        asserteq(ast_node_get_child_count(parent), 4);
        AstNode* expected[4] = {existing[0], existing[1], existing[2], child};
        if (operation != 0) {
          expected[1] = child;
          expected[2] = existing[1];
          expected[3] = existing[2];
        }
        for (size_t index = 0; index < 4; index++) {
          AstNode* actual = NULL;
          ast_node_get_child(ctx, parent, index, &actual);
          asserteq_ptr(actual, expected[index]);
          actual = ast_node_get_parent(expected[index]);
          asserteq_ptr(actual, parent);
        }
      }
    }
  }

  subdesc(APPEND_CHILD) {
    it("Should append the first child and establish its parent") {
      // Arrange: prepare resources for the function.
      AstNode* parent = ast_test_node(ctx);
      AstNode* child = ast_test_node(ctx);

      // Act: execute the function.
      ast_node_append_child(ctx, parent, child);

      // Assert: check the observable result.
      AstNode* actual = NULL;
      ast_node_get_child(ctx, parent, 0, &actual);
      asserteq_ptr(actual, child);
      asserteq(ast_node_get_child_count(parent), 1);
      actual = ast_node_get_parent(child);
      asserteq_ptr(actual, parent);
    }

    it("Should preserve child order across repeated buffer growth") {
      // Arrange: prepare resources for the function.
      AstNode* parent = ast_test_node(ctx);
      AstNode* children[10];
      for (size_t index = 0; index < 10; index++) {
        children[index] = ast_test_node(ctx);
      }

      // Act: execute the function.
      for (size_t index = 0; index < 10; index++) {
        ast_node_append_child(ctx, parent, children[index]);
      }

      // Assert: check the observable result.
      asserteq(ast_node_get_child_count(parent), 10);
      for (size_t index = 0; index < 10; index++) {
        AstNode* actual = NULL;
        ast_node_get_child(ctx, parent, index, &actual);
        asserteq_ptr(actual, children[index]);
        actual = ast_node_get_parent(children[index]);
        asserteq_ptr(actual, parent);
      }
    }
  }

  subdesc(INSERT_CHILD) {
    it("Should insert at index zero into an empty node") {
      // Arrange: prepare resources for the function.
      AstNode* parent = ast_test_node(ctx);
      AstNode* child = ast_test_node(ctx);

      // Act: execute the function.
      ast_node_insert_child(ctx, parent, 0, child);

      // Assert: check the observable result.
      AstNode* actual = NULL;
      ast_node_get_child(ctx, parent, 0, &actual);
      asserteq_ptr(actual, child);
      asserteq(ast_node_get_child_count(parent), 1);
      actual = ast_node_get_parent(child);
      asserteq_ptr(actual, parent);
    }

    it("Should insert at the beginning and preserve child order") {
      // Arrange: prepare resources for the function.
      AstNode* parent = ast_test_node(ctx);
      AstNode* existing[3];
      for (size_t index = 0; index < 3; index++) {
        existing[index] = ast_test_node(ctx);
        ast_node_append_child(ctx, parent, existing[index]);
      }
      AstNode* child = ast_test_node(ctx);

      // Act: execute the function.
      ast_node_insert_child(ctx, parent, 0, child);

      // Assert: check the observable result.
      asserteq(ast_node_get_child_count(parent), 4);
      for (size_t index = 0; index < 4; index++) {
        AstNode* actual = NULL;
        ast_node_get_child(ctx, parent, index, &actual);
        AstNode* expected =
            index == 0 ? child : existing[index < 0 ? index : index - 1];
        asserteq_ptr(actual, expected);
      }
      AstNode* actual_parent = NULL;
      actual_parent = ast_node_get_parent(child);
      asserteq_ptr(actual_parent, parent);
    }

    it("Should insert at the middle and preserve child order") {
      // Arrange: prepare resources for the function.
      AstNode* parent = ast_test_node(ctx);
      AstNode* existing[3];
      for (size_t index = 0; index < 3; index++) {
        existing[index] = ast_test_node(ctx);
        ast_node_append_child(ctx, parent, existing[index]);
      }
      AstNode* child = ast_test_node(ctx);

      // Act: execute the function.
      ast_node_insert_child(ctx, parent, 1, child);

      // Assert: check the observable result.
      asserteq(ast_node_get_child_count(parent), 4);
      for (size_t index = 0; index < 4; index++) {
        AstNode* actual = NULL;
        ast_node_get_child(ctx, parent, index, &actual);
        AstNode* expected =
            index == 1 ? child : existing[index < 1 ? index : index - 1];
        asserteq_ptr(actual, expected);
      }
      AstNode* actual_parent = NULL;
      actual_parent = ast_node_get_parent(child);
      asserteq_ptr(actual_parent, parent);
    }

    it("Should insert at the end and preserve child order") {
      // Arrange: prepare resources for the function.
      AstNode* parent = ast_test_node(ctx);
      AstNode* existing[3];
      for (size_t index = 0; index < 3; index++) {
        existing[index] = ast_test_node(ctx);
        ast_node_append_child(ctx, parent, existing[index]);
      }
      AstNode* child = ast_test_node(ctx);

      // Act: execute the function.
      ast_node_insert_child(ctx, parent, 3, child);

      // Assert: check the observable result.
      asserteq(ast_node_get_child_count(parent), 4);
      for (size_t index = 0; index < 4; index++) {
        AstNode* actual = NULL;
        ast_node_get_child(ctx, parent, index, &actual);
        AstNode* expected =
            index == 3 ? child : existing[index < 3 ? index : index - 1];
        asserteq_ptr(actual, expected);
      }
      AstNode* actual_parent = NULL;
      actual_parent = ast_node_get_parent(child);
      asserteq_ptr(actual_parent, parent);
    }

    it("Should reject insertion beyond the count including SIZE_MAX") {
      // Arrange: prepare resources for the function.
      AstNode* parent = ast_test_node(ctx);
      AstNode* child = ast_test_node(ctx);
      size_t indices[] = {1, SIZE_MAX};

      // Act: execute the function.
      for (size_t index = 0; index < 2; index++) {
        assertThrow(E_INTERNAL_ERROR, {
          ast_node_insert_child(ctx, parent, indices[index], child);
        });
      }

      // Assert: check the observable result.
      asserteq(ast_node_get_child_count(parent), 0);

      AstNode* actual = child;
      actual = ast_node_get_parent(child);
      asserteq_ptr(actual, NULL);
    }
  }

  subdesc(DETACH_CHILD) {
    it("Should detach the only child and clear its parent") {
      // Arrange: prepare resources for the function.
      AstNode* parent = ast_test_node(ctx);
      AstNode* children[1];
      for (size_t index = 0; index < 1; index++) {
        children[index] = ast_test_node(ctx);
        ast_node_append_child(ctx, parent, children[index]);
      }
      AstNode* detached = NULL;

      // Act: execute the function.
      ast_node_detach_child(ctx, parent, 0, &detached);

      // Assert: check the observable result.
      asserteq_ptr(detached, children[0]);
      asserteq(ast_node_get_child_count(parent), 0);
      for (size_t index = 0; index < 0; index++) {
        AstNode* actual = NULL;
        ast_node_get_child(ctx, parent, index, &actual);
        asserteq_ptr(actual, children[index < 0 ? index : index + 1]);
        AstNode* actual_parent = NULL;
        actual_parent = ast_node_get_parent(actual);
        asserteq_ptr(actual_parent, parent);
      }
      AstNode* actual_parent = parent;
      actual_parent = ast_node_get_parent(detached);
      asserteq_ptr(actual_parent, NULL);
      ast_node_set_type(detached, AST_NODE_RETURN);
      asserteq(ast_node_get_type(detached), AST_NODE_RETURN);
    }

    it("Should detach the first child and clear its parent") {
      // Arrange: prepare resources for the function.
      AstNode* parent = ast_test_node(ctx);
      AstNode* children[3];
      for (size_t index = 0; index < 3; index++) {
        children[index] = ast_test_node(ctx);
        ast_node_append_child(ctx, parent, children[index]);
      }
      AstNode* detached = NULL;

      // Act: execute the function.
      ast_node_detach_child(ctx, parent, 0, &detached);

      // Assert: check the observable result.
      asserteq_ptr(detached, children[0]);
      asserteq(ast_node_get_child_count(parent), 2);
      for (size_t index = 0; index < 2; index++) {
        AstNode* actual = NULL;
        ast_node_get_child(ctx, parent, index, &actual);
        asserteq_ptr(actual, children[index < 0 ? index : index + 1]);
        AstNode* actual_parent = NULL;
        actual_parent = ast_node_get_parent(actual);
        asserteq_ptr(actual_parent, parent);
      }
      AstNode* actual_parent = parent;
      actual_parent = ast_node_get_parent(detached);
      asserteq_ptr(actual_parent, NULL);
      ast_node_set_type(detached, AST_NODE_RETURN);
      asserteq(ast_node_get_type(detached), AST_NODE_RETURN);
    }

    it("Should detach the middle child and clear its parent") {
      // Arrange: prepare resources for the function.
      AstNode* parent = ast_test_node(ctx);
      AstNode* children[3];
      for (size_t index = 0; index < 3; index++) {
        children[index] = ast_test_node(ctx);
        ast_node_append_child(ctx, parent, children[index]);
      }
      AstNode* detached = NULL;

      // Act: execute the function.
      ast_node_detach_child(ctx, parent, 1, &detached);

      // Assert: check the observable result.
      asserteq_ptr(detached, children[1]);
      asserteq(ast_node_get_child_count(parent), 2);
      for (size_t index = 0; index < 2; index++) {
        AstNode* actual = NULL;
        ast_node_get_child(ctx, parent, index, &actual);
        asserteq_ptr(actual, children[index < 1 ? index : index + 1]);
        AstNode* actual_parent = NULL;
        actual_parent = ast_node_get_parent(actual);
        asserteq_ptr(actual_parent, parent);
      }
      AstNode* actual_parent = parent;
      actual_parent = ast_node_get_parent(detached);
      asserteq_ptr(actual_parent, NULL);
      ast_node_set_type(detached, AST_NODE_RETURN);
      asserteq(ast_node_get_type(detached), AST_NODE_RETURN);
    }

    it("Should detach the last child and clear its parent") {
      // Arrange: prepare resources for the function.
      AstNode* parent = ast_test_node(ctx);
      AstNode* children[3];
      for (size_t index = 0; index < 3; index++) {
        children[index] = ast_test_node(ctx);
        ast_node_append_child(ctx, parent, children[index]);
      }
      AstNode* detached = NULL;

      // Act: execute the function.
      ast_node_detach_child(ctx, parent, 2, &detached);

      // Assert: check the observable result.
      asserteq_ptr(detached, children[2]);
      asserteq(ast_node_get_child_count(parent), 2);
      for (size_t index = 0; index < 2; index++) {
        AstNode* actual = NULL;
        ast_node_get_child(ctx, parent, index, &actual);
        asserteq_ptr(actual, children[index < 2 ? index : index + 1]);
        AstNode* actual_parent = NULL;
        actual_parent = ast_node_get_parent(actual);
        asserteq_ptr(actual_parent, parent);
      }
      AstNode* actual_parent = parent;
      actual_parent = ast_node_get_parent(detached);
      asserteq_ptr(actual_parent, NULL);
      ast_node_set_type(detached, AST_NODE_RETURN);
      asserteq(ast_node_get_type(detached), AST_NODE_RETURN);
    }

    it("Should reject detachment at the count and SIZE_MAX") {
      // Arrange: prepare resources for the function.
      AstNode* parent = ast_test_node(ctx);
      AstNode* child = ast_test_node(ctx);
      ast_node_append_child(ctx, parent, child);
      size_t indices[] = {1, SIZE_MAX};

      // Act: execute the function.
      for (size_t index = 0; index < 2; index++) {
        AstNode* detached = NULL;
        assertThrow(E_INTERNAL_ERROR, {
          ast_node_detach_child(ctx, parent, indices[index], &detached);
        });
      }

      // Assert: check the observable result.
      asserteq(ast_node_get_child_count(parent), 1);
      AstNode* actual = NULL;
      ast_node_get_child(ctx, parent, 0, &actual);
      asserteq_ptr(actual, child);
    }
  }
}

describe(AST_EXPRESSIONS) {
  subdesc(ATTRIBUTES) {
    it("Should default data type and position of a new node") {
      AstNode* node = NULL;
      ast_node_create(ctx, &node);
      defer({ ast_node_destroy(ctx, node); });

      asserteq(ast_node_get_data_type(node), AST_DATA_TYPE_UNKNOWN);
      asserteq(ast_node_get_line(node), 0);
      asserteq(ast_node_get_column(node), 0);
    }

    it("Should store data type and position independently of the kind") {
      AstNode* node = NULL;
      ast_node_create(ctx, &node);
      defer({ ast_node_destroy(ctx, node); });

      ast_node_set_type(node, AST_NODE_ADD);
      ast_node_set_data_type(node, AST_DATA_TYPE_DOUBLE);
      ast_node_set_position(node, 3, 14);

      asserteq(ast_node_get_type(node), AST_NODE_ADD);
      asserteq(ast_node_get_data_type(node), AST_DATA_TYPE_DOUBLE);
      asserteq(ast_node_get_line(node), 3);
      asserteq(ast_node_get_column(node), 14);
    }

    it("Should report which node types carry a value") {
      asserteq(ast_node_type_has_value(AST_NODE_IDENT), false);
      asserteq(ast_node_type_has_value(AST_NODE_INT_LIT), true);
      asserteq(ast_node_type_has_value(AST_NODE_DOUBLE_LIT), true);
      asserteq(ast_node_type_has_value(AST_NODE_STRING_LIT), true);
      asserteq(ast_node_type_has_value(AST_NODE_BOOL_LIT), true);
      asserteq(ast_node_type_has_value(AST_NODE_ADD), false);
      asserteq(ast_node_type_has_value(AST_NODE_IF), false);
      asserteq(ast_node_type_has_value(AST_NODE_NONE), false);
    }
  }

  subdesc(MAKE_LEAF) {
    it("Should create a literal leaf with its value") {
      AstNode* leaf = NULL;
      ast_make_leaf(ctx, AST_NODE_INT_LIT, (AstNodeValue){.int_value = 42},
                    &leaf);
      defer({ ast_node_destroy(ctx, leaf); });

      asserteq(ast_node_get_type(leaf), AST_NODE_INT_LIT);
      asserteq(ast_node_get_value(leaf).int_value, 42);
      asserteq(ast_node_get_child_count(leaf), 0);
    }

    it("Should reject node types without a value") {
      AstNode* leaf = NULL;
      assertThrow(E_INTERNAL_ERROR, {
        ast_make_leaf(ctx, AST_NODE_ADD, (AstNodeValue){.int_value = 1}, &leaf);
      });
      asserteq_ptr(leaf, NULL);
    }
  }

  subdesc(MAKE_TREE) {
    it("Should build x = a + 1 with children in source order") {
      AstNode* x = NULL;
      AstNode* a = NULL;
      AstNode* one = NULL;
      AstNode* sum = NULL;
      AstNode* assign = NULL;
      ast_node_create(ctx, &x);
      ast_node_set_type(x, AST_NODE_IDENT);
      ast_node_create(ctx, &a);
      ast_node_set_type(a, AST_NODE_IDENT);
      ast_make_leaf(ctx, AST_NODE_INT_LIT, (AstNodeValue){.int_value = 1},
                    &one);

      ast_make_binary(ctx, AST_NODE_ADD, a, one, &sum);
      ast_make_binary(ctx, AST_NODE_ASSIGN, x, sum, &assign);
      defer({ ast_node_destroy(ctx, assign); });

      asserteq(ast_node_get_type(assign), AST_NODE_ASSIGN);
      asserteq(ast_node_get_child_count(assign), 2);
      AstNode* child = NULL;
      ast_node_get_child(ctx, assign, 0, &child);
      asserteq_ptr(child, x);
      ast_node_get_child(ctx, assign, 1, &child);
      asserteq_ptr(child, sum);
      ast_node_get_child(ctx, sum, 0, &child);
      asserteq_ptr(child, a);
      ast_node_get_child(ctx, sum, 1, &child);
      asserteq_ptr(child, one);
    }

    it("Should leave children with the caller when a precondition fails") {
      AstNode* left = NULL;
      AstNode* right = NULL;
      AstNode* parent = NULL;
      AstNode* result = NULL;
      ast_node_create(ctx, &left);
      ast_node_create(ctx, &right);
      ast_node_create(ctx, &parent);
      ast_node_append_child(ctx, parent, right);
      defer({
        ast_node_destroy(ctx, parent);
        ast_node_destroy(ctx, left);
      });

      assertThrow(E_INTERNAL_ERROR, {
        ast_make_binary(ctx, AST_NODE_ADD, left, right, &result);
      });
      ctx->exit_code = E_OK;
      assertThrow(E_INTERNAL_ERROR,
                  { ast_make_binary(ctx, AST_NODE_ADD, left, NULL, &result); });

      asserteq_ptr(result, NULL);
      AstNode* actual_parent = left;
      actual_parent = ast_node_get_parent(left);
      asserteq_ptr(actual_parent, NULL);
      actual_parent = ast_node_get_parent(right);
      asserteq_ptr(actual_parent, parent);
    }

    it("Should build a unary node") {
      AstNode* operand = NULL;
      AstNode* negation = NULL;
      ast_make_leaf(ctx, AST_NODE_BOOL_LIT,
                    (AstNodeValue){.boolean_value = true}, &operand);
      ast_make_unary(ctx, AST_NODE_NOT, operand, &negation);
      defer({ ast_node_destroy(ctx, negation); });

      asserteq(ast_node_get_type(negation), AST_NODE_NOT);
      asserteq(ast_node_get_child_count(negation), 1);
      AstNode* child = NULL;
      ast_node_get_child(ctx, negation, 0, &child);
      asserteq_ptr(child, operand);
    }
  }
}

describe(AST_NODE_DATA) {
  subdesc(NAME) {
    it("Should keep its own copy of the identifier name") {
      AstNode* ident = NULL;
      ast_node_create(ctx, &ident);
      defer({ ast_node_destroy(ctx, ident); });
      char token_text[] = "counter";

      ast_node_set_name(ctx, ident, token_text);
      token_text[0] = 'X';

      assertneq_ptr(ast_node_get_name(ident), token_text);
      asserteq_str(ast_node_get_name(ident), "counter");
    }

    it("Should replace the name and clear it with NULL") {
      AstNode* ident = NULL;
      ast_node_create(ctx, &ident);
      defer({ ast_node_destroy(ctx, ident); });
      asserteq_ptr(ast_node_get_name(ident), NULL);

      ast_node_set_name(ctx, ident, "first");
      ast_node_set_name(ctx, ident, "second");
      asserteq_str(ast_node_get_name(ident), "second");

      ast_node_set_name(ctx, ident, NULL);
      asserteq_ptr(ast_node_get_name(ident), NULL);
    }

    it("Should keep the previous name when copying fails") {
      AstNode* ident = NULL;
      ast_node_create(ctx, &ident);
      defer({ ast_node_destroy(ctx, ident); });
      ast_node_set_name(ctx, ident, "kept");

      ctx->flags.forceNthMallocFail = 1;
      assertThrow(E_INTERNAL_ERROR,
                  { ast_node_set_name(ctx, ident, "replacement"); });
      ctx->exit_code = E_OK;

      asserteq_str(ast_node_get_name(ident), "kept");
    }
  }

  subdesc(SYMBOL) {
    it("Should resolve the symbol later without touching the name") {
      static int symbol_storage;
      const SymtableData* symbol = (const SymtableData*)&symbol_storage;
      AstNode* ident = NULL;
      ast_node_create(ctx, &ident);
      defer({ ast_node_destroy(ctx, ident); });
      ast_node_set_name(ctx, ident, "factorial");
      asserteq_ptr(ast_node_get_symbol(ident), NULL);

      ast_node_set_symbol(ident, symbol);

      asserteq_ptr(ast_node_get_symbol(ident), symbol);
      asserteq_str(ast_node_get_name(ident), "factorial");
    }
  }

  subdesc(STRING_VALUE) {
    it("Should keep its own copy of a string literal after the token is "
       "freed") {
      char* token_text = malloc(sizeof("hello"));
      assertneq_ptr(token_text, NULL);
      memcpy(token_text, "hello", sizeof("hello"));
      AstNode* literal = NULL;

      ast_make_leaf(ctx, AST_NODE_STRING_LIT,
                    (AstNodeValue){.string_value = token_text}, &literal);
      defer({ ast_node_destroy(ctx, literal); });
      token_text[0] = 'X';
      free(token_text);

      asserteq_str(ast_node_get_value(literal).string_value, "hello");
    }

    it("Should not own a value set before the node became a string literal") {
      AstNode* node = NULL;
      ast_node_create(ctx, &node);
      defer({ ast_node_destroy(ctx, node); });
      ast_node_set_type(node, AST_NODE_INT_LIT);
      ast_node_set_value(ctx, node, (AstNodeValue){.int_value = 42});

      ast_node_set_type(node, AST_NODE_STRING_LIT);

      asserteq_ptr(ast_node_get_value(node).string_value, NULL);
    }
  }

  subdesc(FLAGS) {
    it("Should start without flags and store combined flags") {
      AstNode* def = NULL;
      ast_node_create(ctx, &def);
      defer({ ast_node_destroy(ctx, def); });
      asserteq(ast_node_get_flags(def), 0);

      ast_node_set_flags(def, AST_FLAG_MAIN | AST_FLAG_EXPR);

      asserteq(ast_node_get_flags(def) & AST_FLAG_MAIN, AST_FLAG_MAIN);
      asserteq(ast_node_get_flags(def) & AST_FLAG_EXPR, AST_FLAG_EXPR);
    }
  }
}

// AI generated by: Claude Code (claude-sonnet-5-5)
// Prompt: "implementation done using skill: mattpocock-skills:implement-spec,
// ticket 01: prefactor, AST scope ownership and shared test helpers"
// Changes:
// Added tests for the scope a BLOCK, DEF or FOR node owns: attaching,
// refusing a second scope, destroying nodes that own scopes and a forced
// allocation failure while creating one.
describe(AST_SCOPE) {
  subdesc(ATTACH) {
    it("Should start without a scope") {
      AstNode* block = NULL;
      ast_node_create(ctx, &block);
      defer({ ast_node_destroy(ctx, block); });

      asserteq_ptr(ast_node_get_scope(block), NULL);
    }

    it("Should attach a new scope with the given parent and return it") {
      AstNode* block = NULL;
      ast_node_create(ctx, &block);
      defer({ ast_node_destroy(ctx, block); });

      Symtable* scope =
          ast_node_attach_scope(ctx, block, &ctx->global_symtable);

      assertneq_ptr(scope, NULL);
      asserteq_ptr(ast_node_get_scope(block), scope);
      asserteq_ptr(scope->parent, &ctx->global_symtable);
    }

    it("Should attach a scope with no parent") {
      AstNode* block = NULL;
      ast_node_create(ctx, &block);
      defer({ ast_node_destroy(ctx, block); });

      Symtable* scope = ast_node_attach_scope(ctx, block, NULL);

      asserteq_ptr(scope->parent, NULL);
    }

    it("Should give every node its own scope") {
      AstNode* outer = NULL;
      AstNode* inner = NULL;
      ast_node_create(ctx, &outer);
      defer({ ast_node_destroy(ctx, outer); });
      ast_node_create(ctx, &inner);
      ast_node_append_child(ctx, outer, inner);

      Symtable* outer_scope =
          ast_node_attach_scope(ctx, outer, &ctx->global_symtable);
      Symtable* inner_scope = ast_node_attach_scope(ctx, inner, outer_scope);

      assertneq_ptr(outer_scope, inner_scope);
      asserteq_ptr(ast_node_get_scope(outer), outer_scope);
      asserteq_ptr(ast_node_get_scope(inner), inner_scope);
      asserteq_ptr(inner_scope->parent, outer_scope);
    }

    it("Should refuse a node that already has a scope and keep the first") {
      AstNode* block = NULL;
      ast_node_create(ctx, &block);
      defer({ ast_node_destroy(ctx, block); });
      Symtable* first = ast_node_attach_scope(ctx, block, NULL);

      assertThrow(E_INTERNAL_ERROR, {
        ast_node_attach_scope(ctx, block, &ctx->global_symtable);
      });
      ctx->exit_code = E_OK;

      asserteq_ptr(ast_node_get_scope(block), first);
      asserteq_ptr(first->parent, NULL);
    }

    it("Should refuse a NULL node") {
      assertThrow(E_INTERNAL_ERROR,
                  { ast_node_attach_scope(ctx, NULL, NULL); });
      ctx->exit_code = E_OK;
    }
  }

  subdesc(DESTROY) {
    it("Should destroy the scope with the node, symbols included") {
      AstNode* block = NULL;
      ast_node_create(ctx, &block);
      Symtable* scope =
          ast_node_attach_scope(ctx, block, &ctx->global_symtable);
      symtable_insert(ctx, scope, "x",
                      (SymtableData){.symbol_type = SYMBOL_VARIABLE});
      symtable_insert(ctx, scope, "y",
                      (SymtableData){.symbol_type = SYMBOL_VARIABLE});

      // leaks and the sanitizers report anything the node leaves behind
      assertNoThrow({ ast_node_destroy(ctx, block); });
    }

    it("Should destroy the scopes of a whole subtree") {
      AstNode* program = NULL;
      AstNode* def = NULL;
      AstNode* body = NULL;
      AstNode* nested = NULL;
      ast_node_create(ctx, &program);
      ast_node_create(ctx, &def);
      ast_node_create(ctx, &body);
      ast_node_create(ctx, &nested);
      ast_node_append_child(ctx, program, def);
      ast_node_append_child(ctx, def, body);
      ast_node_append_child(ctx, body, nested);
      Symtable* params = ast_node_attach_scope(ctx, def, &ctx->global_symtable);
      Symtable* body_scope = ast_node_attach_scope(ctx, body, params);
      Symtable* nested_scope = ast_node_attach_scope(ctx, nested, body_scope);
      symtable_insert(ctx, params, "n",
                      (SymtableData){.symbol_type = SYMBOL_VARIABLE});
      symtable_insert(ctx, body_scope, "x",
                      (SymtableData){.symbol_type = SYMBOL_VARIABLE});
      symtable_insert(ctx, nested_scope, "x",
                      (SymtableData){.symbol_type = SYMBOL_VARIABLE});

      assertNoThrow({ ast_node_destroy(ctx, program); });
    }

    it("Should keep the scope of a detached subtree alive until it is "
       "destroyed") {
      AstNode* parent = NULL;
      AstNode* block = NULL;
      ast_node_create(ctx, &parent);
      defer({ ast_node_destroy(ctx, parent); });
      ast_node_create(ctx, &block);
      ast_node_append_child(ctx, parent, block);
      Symtable* scope = ast_node_attach_scope(ctx, block, NULL);

      AstNode* detached = NULL;
      ast_node_detach_child(ctx, parent, 0, &detached);

      asserteq_ptr(ast_node_get_scope(detached), scope);
      ast_node_destroy(ctx, detached);
    }
  }

  subdesc(ALLOCATION_FAILURE) {
    it("Should leak nothing and attach nothing when creating a scope fails") {
      AstNode* block = NULL;
      ast_node_create(ctx, &block);
      defer({ ast_node_destroy(ctx, block); });

      ctx->flags.forceNthMallocFail = 1;
      assertThrow(E_INTERNAL_ERROR, {
        ast_node_attach_scope(ctx, block, &ctx->global_symtable);
      });
      ctx->exit_code = E_OK;

      asserteq_ptr(ast_node_get_scope(block), NULL);
      // The failed attempt did not use the node up: a retry works
      Symtable* scope = ast_node_attach_scope(ctx, block, NULL);
      asserteq_ptr(ast_node_get_scope(block), scope);
    }
  }
}

// AI generated by: Claude Code (claude-sonnet-5-5)
// Prompt: "implementation done using skill: mattpocock-skills:implement-spec,
// ticket 01: prefactor, AST scope ownership and shared test helpers"
// Changes:
// Added tests for the INT_TO_DOUBLE node kind and the BUILTIN and DEF checking
// flags semantic analysis writes.
describe(AST_SEMANTIC_ANNOTATIONS) {
  subdesc(INT_TO_DOUBLE) {
    it("Should wrap an expression like a unary operator does") {
      AstNode* literal = NULL;
      ast_make_leaf(ctx, AST_NODE_INT_LIT, (AstNodeValue){.int_value = 1},
                    &literal);
      AstNode* widened = NULL;
      ast_make_unary(ctx, AST_NODE_INT_TO_DOUBLE, literal, &widened);
      defer({ ast_node_destroy(ctx, widened); });

      asserteq(ast_node_get_type(widened), AST_NODE_INT_TO_DOUBLE);
      asserteq(ast_node_get_child_count(widened), 1);
      asserteq_ptr(ast_node_get_parent(literal), widened);
      ast_node_set_data_type(widened, AST_DATA_TYPE_DOUBLE);
      asserteq(ast_node_get_data_type(widened), AST_DATA_TYPE_DOUBLE);
    }

    it("Should replace a child in place by detach, wrap and insert") {
      AstNode* add = NULL;
      AstNode* left = NULL;
      AstNode* right = NULL;
      ast_make_leaf(ctx, AST_NODE_INT_LIT, (AstNodeValue){.int_value = 1},
                    &left);
      ast_make_leaf(ctx, AST_NODE_DOUBLE_LIT, (AstNodeValue){.double_value = 2},
                    &right);
      ast_make_binary(ctx, AST_NODE_ADD, left, right, &add);
      defer({ ast_node_destroy(ctx, add); });

      AstNode* detached = NULL;
      AstNode* widened = NULL;
      ast_node_detach_child(ctx, add, 0, &detached);
      ast_make_unary(ctx, AST_NODE_INT_TO_DOUBLE, detached, &widened);
      ast_node_insert_child(ctx, add, 0, widened);

      AstNode* first = NULL;
      ast_node_get_child(ctx, add, 0, &first);
      asserteq_ptr(first, widened);
      asserteq(ast_node_get_child_count(add), 2);
    }
  }

  subdesc(FLAGS) {
    it("Should keep BUILTIN and the two DEF flags apart from each other and "
       "from MAIN and EXPR") {
      unsigned all[] = {AST_FLAG_MAIN, AST_FLAG_EXPR, AST_FLAG_BUILTIN,
                        AST_FLAG_DEF_CHECKING, AST_FLAG_DEF_CHECKED};
      unsigned seen = 0;
      for (size_t i = 0; i < sizeof(all) / sizeof(all[0]); i++) {
        asserteq(all[i] & (all[i] - 1), 0u);  // a single bit
        asserteq(seen & all[i], 0u);
        seen |= all[i];
      }
    }

    it("Should store the new flags together with the old ones") {
      AstNode* def = NULL;
      ast_node_create(ctx, &def);
      defer({ ast_node_destroy(ctx, def); });

      ast_node_set_flags(def, AST_FLAG_MAIN | AST_FLAG_DEF_CHECKING);
      ast_node_set_flags(def, ast_node_get_flags(def) | AST_FLAG_DEF_CHECKED);

      unsigned flags = ast_node_get_flags(def);
      assert(flags & AST_FLAG_MAIN);
      assert(flags & AST_FLAG_DEF_CHECKING);
      assert(flags & AST_FLAG_DEF_CHECKED);
      assert(!(flags & AST_FLAG_BUILTIN));
    }

    it("Should mark a call as resolved to a built-in") {
      AstNode* call = NULL;
      ast_node_create(ctx, &call);
      defer({ ast_node_destroy(ctx, call); });

      ast_node_set_flags(call, AST_FLAG_BUILTIN);

      asserteq(ast_node_get_flags(call), AST_FLAG_BUILTIN);
    }
  }
}

// AI generated by: Claude Code (claude-sonnet-5-5)
// Prompt: "implementation done using skill: mattpocock-skills:implement-spec,
// ticket 01: prefactor, AST scope ownership and shared test helpers"
// Changes:
// Added tests for the annotations the shared AST test printer shows when they
// are set (data type, slot, function id, built-in, BUILTIN flag,
// INT_TO_DOUBLE) and for what it never shows (the DEF checking flags).
static AstNode* printer_leaf(Ctx* ctx, AstNodeType type, int value) {
  AstNode* node = NULL;
  ast_make_leaf(ctx, type, (AstNodeValue){.int_value = value}, &node);
  return node;
}

static AstNode* printer_ident(Ctx* ctx,
                              const char* name,
                              const SymtableData* symbol) {
  AstNode* node = NULL;
  ast_node_create(ctx, &node);
  ast_node_set_type(node, AST_NODE_IDENT);
  ast_node_set_name(ctx, node, name);
  ast_node_set_symbol(node, symbol);
  return node;
}

describe(AST_TEST_STRING) {
  subdesc(UNANNOTATED) {
    it("Should print an AST the parser produces without any annotation") {
      AstNode* add = NULL;
      ast_make_binary(ctx, AST_NODE_ADD, printer_leaf(ctx, AST_NODE_INT_LIT, 1),
                      printer_ident(ctx, "x", NULL), &add);
      defer({ ast_node_destroy(ctx, add); });
      ast_node_set_flags(add, AST_FLAG_EXPR);

      char* text = ast_to_test_string(add);
      defer(free(text));

      asserteq_str(text, "(ADD [EXPR] (INT_LIT 1) (IDENT x))");
    }

    it("Should print a type annotation's type once, not as a data type") {
      AstNode* annotation = NULL;
      ast_node_create(ctx, &annotation);
      defer({ ast_node_destroy(ctx, annotation); });
      ast_node_set_type(annotation, AST_NODE_TYPE_ANNOTATION);
      ast_node_set_data_type(annotation, AST_DATA_TYPE_INT);

      char* text = ast_to_test_string(annotation);
      defer(free(text));

      asserteq_str(text, "(TYPE_ANNOTATION Int)");
    }
  }

  subdesc(DATA_TYPE) {
    it("Should print the data type when set") {
      AstNode* literal = printer_leaf(ctx, AST_NODE_INT_LIT, 7);
      defer({ ast_node_destroy(ctx, literal); });
      ast_node_set_data_type(literal, AST_DATA_TYPE_INT);

      char* text = ast_to_test_string(literal);
      defer(free(text));

      asserteq_str(text, "(INT_LIT 7 :Int)");
    }

    it("Should print every data type by name") {
      AstDataType types[] = {AST_DATA_TYPE_INT, AST_DATA_TYPE_DOUBLE,
                             AST_DATA_TYPE_BOOLEAN, AST_DATA_TYPE_STRING,
                             AST_DATA_TYPE_UNIT};
      const char* expected[] = {"(UNIT_LIT :Int)", "(UNIT_LIT :Double)",
                                "(UNIT_LIT :Boolean)", "(UNIT_LIT :String)",
                                "(UNIT_LIT :Unit)"};
      for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        AstNode* node = NULL;
        ast_node_create(ctx, &node);
        ast_node_set_type(node, AST_NODE_UNIT_LIT);
        ast_node_set_data_type(node, types[i]);

        // no defer in a loop: it would run after the last iteration only
        char* text = ast_to_test_string(node);
        ast_node_destroy(ctx, node);
        asserteq_str(text, expected[i]);
        free(text);
      }
    }

    it("Should print INT_TO_DOUBLE wrapping its Int expression") {
      AstNode* literal = printer_leaf(ctx, AST_NODE_INT_LIT, 1);
      ast_node_set_data_type(literal, AST_DATA_TYPE_INT);
      AstNode* widened = NULL;
      ast_make_unary(ctx, AST_NODE_INT_TO_DOUBLE, literal, &widened);
      defer({ ast_node_destroy(ctx, widened); });
      ast_node_set_data_type(widened, AST_DATA_TYPE_DOUBLE);

      char* text = ast_to_test_string(widened);
      defer(free(text));

      asserteq_str(text, "(INT_TO_DOUBLE :Double (INT_LIT 1 :Int))");
    }
  }

  subdesc(SYMBOLS) {
    it("Should print a variable symbol's slot") {
      SymtableData variable = {.symbol_type = SYMBOL_VARIABLE, .ir_index = 3};
      AstNode* ident = printer_ident(ctx, "x", &variable);
      defer({ ast_node_destroy(ctx, ident); });
      ast_node_set_data_type(ident, AST_DATA_TYPE_INT);

      char* text = ast_to_test_string(ident);
      defer(free(text));

      asserteq_str(text, "(IDENT x :Int @3)");
    }

    it("Should print slot 0 too, it is a slot") {
      SymtableData variable = {.symbol_type = SYMBOL_VARIABLE, .ir_index = 0};
      AstNode* ident = printer_ident(ctx, "x", &variable);
      defer({ ast_node_destroy(ctx, ident); });

      char* text = ast_to_test_string(ident);
      defer(free(text));

      asserteq_str(text, "(IDENT x @0)");
    }

    it("Should tell two shadowed variables apart by their slots") {
      SymtableData outer = {.symbol_type = SYMBOL_VARIABLE, .ir_index = 1};
      SymtableData inner = {.symbol_type = SYMBOL_VARIABLE, .ir_index = 2};
      AstNode* add = NULL;
      ast_make_binary(ctx, AST_NODE_ADD, printer_ident(ctx, "x", &outer),
                      printer_ident(ctx, "x", &inner), &add);
      defer({ ast_node_destroy(ctx, add); });

      char* text = ast_to_test_string(add);
      defer(free(text));

      asserteq_str(text, "(ADD (IDENT x @1) (IDENT x @2))");
    }

    it("Should print a user function symbol's function id") {
      SymtableData function = {.symbol_type = SYMBOL_FUNCTION, .ir_index = 2};
      AstNode* ident = printer_ident(ctx, "f", &function);
      defer({ ast_node_destroy(ctx, ident); });

      char* text = ast_to_test_string(ident);
      defer(free(text));

      asserteq_str(text, "(IDENT f fn2)");
    }

    // AI generated by: Claude Code (claude-opus-5-5)
    // Prompt: "implementation done using skill:
    // mattpocock-skills:implement-spec, ticket 01: prefactor, AST scope
    // ownership and shared test helpers"
    // Changes:
    // Review fix: the real shape, symbol on the callee and BUILTIN on the CALL
    it("Should print a callee of a BUILTIN call as a built-in") {
      SymtableData builtin = {.symbol_type = SYMBOL_FUNCTION, .ir_index = 5};
      AstNode* call = NULL;
      ast_node_create(ctx, &call);
      defer({ ast_node_destroy(ctx, call); });
      ast_node_set_type(call, AST_NODE_CALL);
      ast_node_set_flags(call, AST_FLAG_BUILTIN);
      AstNode* callee = printer_ident(ctx, "print", &builtin);
      ast_node_append_child(ctx, call, callee);

      char* text = ast_to_test_string(call);
      defer(free(text));

      asserteq_str(text, "(CALL [BUILTIN] (IDENT print builtin))");
    }

    it("Should print a built-in callee as a built-in when it carries BUILTIN") {
      SymtableData builtin = {.symbol_type = SYMBOL_FUNCTION, .ir_index = 5};
      AstNode* ident = printer_ident(ctx, "print", &builtin);
      defer({ ast_node_destroy(ctx, ident); });
      ast_node_set_flags(ident, AST_FLAG_BUILTIN);

      char* text = ast_to_test_string(ident);
      defer(free(text));

      asserteq_str(text, "(IDENT print [BUILTIN] builtin)");
    }
  }

  subdesc(DEF_CHECKING_FLAGS) {
    it("Should never print the DEF checking flags") {
      AstNode* def = NULL;
      ast_node_create(ctx, &def);
      defer({ ast_node_destroy(ctx, def); });
      ast_node_set_type(def, AST_NODE_DEF);
      ast_node_set_flags(def, AST_FLAG_DEF_CHECKING | AST_FLAG_DEF_CHECKED);

      char* text = ast_to_test_string(def);
      defer(free(text));

      asserteq_str(text, "(DEF)");
    }

    it("Should print the other flags next to the DEF checking flags") {
      AstNode* def = NULL;
      ast_node_create(ctx, &def);
      defer({ ast_node_destroy(ctx, def); });
      ast_node_set_type(def, AST_NODE_DEF);
      ast_node_set_flags(def, AST_FLAG_MAIN | AST_FLAG_DEF_CHECKED);

      char* text = ast_to_test_string(def);
      defer(free(text));

      asserteq_str(text, "(DEF [MAIN])");
    }
  }
}

snow_main();
