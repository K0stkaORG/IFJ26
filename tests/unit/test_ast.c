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

snow_main();
