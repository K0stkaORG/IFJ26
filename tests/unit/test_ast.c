/**
 * @file ast.h
 * Project: IFJ26
 *
 * @author Lukáš Pollák (xpollal00)
 * @date 31. 10. 2026
 *
 * @brief Unit test for the AST
 */

#include <float.h>
#include <limits.h>
#include <stdint.h>
#include "ast.h"
#include "snow/snow.h"

#define AST_TEST_NODE_CAPACITY 32
#define AST_TEST_NODE_TYPE_COUNT 24
#define AST_TEST_INT_VALUE_COUNT 4
#define AST_TEST_CHAR_VALUE_COUNT 4
#define AST_TEST_DOUBLE_VALUE_COUNT 4
#define AST_TEST_BOOL_VALUE_COUNT 3
#define AST_TEST_STRING_VALUE_COUNT 3

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
      ast_node_get_parent(node, &parent);
      asserteq_ptr(parent, NULL);
    }

    it("Should throw an error when node address is NULL") {
      // Arrange: use the initialized test context and a NULL output pointer.

      // Act: attempt creation without an output address.
      assertThrow(E_INTERNAL_ERROR, { ast_node_create(ctx, NULL); });

      // Assert: check the reported error code.
      asserteq(ctx->exit_code, E_INTERNAL_ERROR);
    }

    it("Should throw an error when allocation fails") {
      // Arrange: prepare cleanup and force the next allocation to fail.
      static AstNode* node;
      node = NULL;
      defer({
        if (node != NULL) {
          ast_node_destroy(ctx, node);
        }
      });

      ctx->flags.forceNthMallocFail = 1;

      // Act: attempt to allocate a node.
      assertThrow(E_INTERNAL_ERROR, { ast_node_create(ctx, &node); });

      // Assert: check the reported error code.
      asserteq(ctx->exit_code, E_INTERNAL_ERROR);
    }
  }

  subdesc(DESTROY) {
    it("Should destroy a node without children") {
      // Arrange: create a leaf and register fallback cleanup.
      static AstNode* node;
      node = NULL;
      defer({
        if (node != NULL) {
          ast_node_destroy(ctx, node);
        }
      });

      ast_node_create(ctx, &node);

      // Act: destroy the leaf and release the cleanup reference.
      assertNoThrow({
        ast_node_destroy(ctx, node);
        node = NULL;
      });

      // Assert: check that destruction completed without an error.
      asserteq(ctx->exit_code, E_OK);
    }

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

    it("Should throw an error when node is NULL") {
      // Arrange: use the initialized test context and a NULL node.

      // Act: attempt to destroy a NULL node.
      assertThrow(E_INTERNAL_ERROR, { ast_node_destroy(ctx, NULL); });

      // Assert: check the reported error code.
      asserteq(ctx->exit_code, E_INTERNAL_ERROR);
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

  subdesc(TYPE) {
    it("Should test out every declared node type") {
      // Arrange: prepare resources for the function.
      AstNode* node = ast_test_node(ctx);
      AstNodeType types[AST_TEST_NODE_TYPE_COUNT] = {
          AST_NODE_INVALID,     AST_NODE_NONE,      AST_NODE_IMPORT,
          AST_NODE_DEF,         AST_NODE_RETURN,    AST_NODE_CASE,
          AST_NODE_MATCH,       AST_NODE_IF,        AST_NODE_ELSE,
          AST_NODE_THEN,        AST_NODE_DO,        AST_NODE_VAL,
          AST_NODE_VAR,         AST_NODE_FOR,       AST_NODE_WHILE,
          AST_NODE_TYPE_DOUBLE, AST_NODE_TYPE_INT,  AST_NODE_TYPE_BOOLEAN,
          AST_NODE_TYPE_STRING, AST_NODE_TYPE_UNIT, AST_NODE_CLASS,
          AST_NODE_EXTENSION,   AST_NODE_TYPE_LIST, AST_NODE_TYPE};
      AstNodeType actual[AST_TEST_NODE_TYPE_COUNT];

      // Act: execute the function.
      for (size_t index = 0; index < AST_TEST_NODE_TYPE_COUNT; index++) {
        ast_node_set_type(node, types[index]);
        actual[index] = ast_node_get_type(node);
      }

      // Assert: check the observable result.
      for (size_t index = 0; index < AST_TEST_NODE_TYPE_COUNT; index++) {
        asserteq(actual[index], types[index]);
      }
      asserteq(ast_node_get_child_count(node), 0);
    }
  }

  subdesc(VALUE) {
    it("Should test integer values and boundaries") {
      // Arrange: prepare resources for the function.
      AstNode* node = ast_test_node(ctx);
      int values[AST_TEST_INT_VALUE_COUNT] = {0, -1, INT_MIN, INT_MAX};
      AstNodeValue actual[AST_TEST_INT_VALUE_COUNT];

      // Act: execute the function.
      for (size_t index = 0; index < AST_TEST_INT_VALUE_COUNT; index++) {
        ast_node_set_value(node, (AstNodeValue){.int_value = values[index]});
        actual[index] = ast_node_get_value(node);
      }

      // Assert: check the observable result.
      for (size_t index = 0; index < AST_TEST_INT_VALUE_COUNT; index++) {
        asserteq(actual[index].int_value, values[index]);
      }
      asserteq(ast_node_get_type(node), AST_NODE_NONE);
    }

    it("Should test character values and boundaries") {
      // Arrange: prepare resources for the function.
      AstNode* node = ast_test_node(ctx);
      char values[AST_TEST_CHAR_VALUE_COUNT] = {'\0', 'A', CHAR_MIN, CHAR_MAX};
      AstNodeValue actual[AST_TEST_CHAR_VALUE_COUNT];

      // Act: execute the function.
      for (size_t index = 0; index < AST_TEST_CHAR_VALUE_COUNT; index++) {
        ast_node_set_value(node, (AstNodeValue){.char_value = values[index]});
        actual[index] = ast_node_get_value(node);
      }

      // Assert: check the observable result.
      for (size_t index = 0; index < AST_TEST_CHAR_VALUE_COUNT; index++) {
        asserteq(actual[index].char_value, values[index]);
      }
      asserteq(ast_node_get_type(node), AST_NODE_NONE);
    }

    it("Should test double values and boundaries") {
      // Arrange: prepare resources for the function.
      AstNode* node = ast_test_node(ctx);
      double values[AST_TEST_DOUBLE_VALUE_COUNT] = {0.0, -2.25, 1.5, DBL_MAX};
      AstNodeValue actual[AST_TEST_DOUBLE_VALUE_COUNT];

      // Act: execute the function.
      for (size_t index = 0; index < AST_TEST_DOUBLE_VALUE_COUNT; index++) {
        ast_node_set_value(node, (AstNodeValue){.float_value = values[index]});
        actual[index] = ast_node_get_value(node);
      }

      // Assert: check the observable result.
      for (size_t index = 0; index < AST_TEST_DOUBLE_VALUE_COUNT; index++) {
        asserteq_dbl(actual[index].float_value, values[index]);
      }
      asserteq(ast_node_get_type(node), AST_NODE_NONE);
    }

    it("Should test boolean values and boundaries") {
      // Arrange: prepare resources for the function.
      AstNode* node = ast_test_node(ctx);
      bool values[AST_TEST_BOOL_VALUE_COUNT] = {false, true, false};
      AstNodeValue actual[AST_TEST_BOOL_VALUE_COUNT];

      // Act: execute the function.
      for (size_t index = 0; index < AST_TEST_BOOL_VALUE_COUNT; index++) {
        ast_node_set_value(node,
                           (AstNodeValue){.boolean_value = values[index]});
        actual[index] = ast_node_get_value(node);
      }

      // Assert: check the observable result.
      for (size_t index = 0; index < AST_TEST_BOOL_VALUE_COUNT; index++) {
        asserteq(actual[index].boolean_value, values[index]);
      }
      asserteq(ast_node_get_type(node), AST_NODE_NONE);
    }

    it("Should test string pointer values and boundaries") {
      // Arrange: prepare resources for the function.
      AstNode* node = ast_test_node(ctx);
      const char* values[AST_TEST_STRING_VALUE_COUNT] = {"hello", "", NULL};
      AstNodeValue actual[AST_TEST_STRING_VALUE_COUNT];

      // Act: execute the function.
      for (size_t index = 0; index < AST_TEST_STRING_VALUE_COUNT; index++) {
        ast_node_set_value(node, (AstNodeValue){.string_value = values[index]});
        actual[index] = ast_node_get_value(node);
      }

      // Assert: check the observable result.
      for (size_t index = 0; index < AST_TEST_STRING_VALUE_COUNT; index++) {
        asserteq_ptr(actual[index].string_value, values[index]);
      }
      asserteq(ast_node_get_type(node), AST_NODE_NONE);
    }
  }

  subdesc(CHILD_COUNT) {
    it("New node should have zero children") {
      // Arrange: prepare resources for the function.
      AstNode* node = ast_test_node(ctx);

      // Act: execute the function.
      size_t actual = ast_node_get_child_count(node);

      // Assert: check the observable result.
      asserteq(actual, 0);
    }

    it("Node should have correct child count after attachment and detachment") {
      // Arrange: prepare resources for the function.
      AstNode* parent = ast_test_node(ctx);
      AstNode* first = ast_test_node(ctx);
      AstNode* second = ast_test_node(ctx);
      AstNode* detached = NULL;

      // Act: execute the function.
      ast_node_append_child(ctx, parent, first);
      size_t after_append = ast_node_get_child_count(parent);
      ast_node_insert_child(ctx, parent, 0, second);
      size_t after_insert = ast_node_get_child_count(parent);
      ast_node_detach_child(ctx, parent, 1, &detached);
      size_t after_detach = ast_node_get_child_count(parent);

      // Assert: check the observable result.
      asserteq(after_append, 1);
      asserteq(after_insert, 2);
      asserteq(after_detach, 1);
      asserteq_ptr(detached, first);
    }
  }

  subdesc(GET_CHILD) {
    it("Should get the first middle and last children in order") {
      // Arrange: prepare resources for the function.
      AstNode* parent = ast_test_node(ctx);
      AstNode* expected[3];
      AstNode* actual[3] = {NULL, NULL, NULL};
      for (size_t index = 0; index < 3; index++) {
        expected[index] = ast_test_node(ctx);
        ast_node_append_child(ctx, parent, expected[index]);
      }

      // Act: execute the function.
      for (size_t index = 0; index < 3; index++) {
        ast_node_get_child(ctx, parent, index, &actual[index]);
      }

      // Assert: check the observable result.
      for (size_t index = 0; index < 3; index++) {
        asserteq_ptr(actual[index], expected[index]);
      }
    }

    it("Should reject index zero on an empty node") {
      // Arrange: prepare resources for the function.
      AstNode* parent = ast_test_node(ctx);
      AstNode* child = NULL;

      // Act: execute the function.
      assertThrow(E_INTERNAL_ERROR,
                  { ast_node_get_child(ctx, parent, 0, &child); });

      // Assert: check the observable result.
      asserteq(ctx->exit_code, E_INTERNAL_ERROR);
      asserteq(ast_node_get_child_count(parent), 0);
    }

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

  subdesc(GET_PARENT) {
    it("Parent should be NULL for root node") {
      // Arrange: prepare resources for the function.
      AstNode* node = ast_test_node(ctx);
      AstNode* parent = node;

      // Act: execute the function.
      ast_node_get_parent(node, &parent);

      // Assert: check the observable result.
      asserteq_ptr(parent, NULL);
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
      ast_node_get_parent(child, &actual);
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
        ast_node_get_parent(children[index], &actual);
        asserteq_ptr(actual, parent);
      }
    }

    it("Should reject a NULL append parent without attaching the child") {
      // Arrange: prepare resources for the function.
      AstNode* child = ast_test_node(ctx);

      // Act: execute the function.
      assertThrow(E_INTERNAL_ERROR,
                  { ast_node_append_child(ctx, NULL, child); });

      // Assert: check the observable result.
      AstNode* actual = child;
      ast_node_get_parent(child, &actual);
      asserteq_ptr(actual, NULL);
    }

    it("Should throw on append allocation failure") {
      // Arrange: prepare resources for the function.
      AstNode* parent = ast_test_node(ctx);
      AstNode* child = ast_test_node(ctx);
      ctx->flags.forceNthMallocFail = 1;

      // Act: execute the function.
      assertThrow(E_INTERNAL_ERROR,
                  { ast_node_append_child(ctx, parent, child); });

      // Assert: check the observable result.
      asserteq(ctx->flags.forceNthMallocFail, 0);
      asserteq(ast_node_get_child_count(parent), 0);

      AstNode* actual = child;
      ast_node_get_parent(child, &actual);
      asserteq_ptr(actual, NULL);
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
      ast_node_get_parent(child, &actual);
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
      ast_node_get_parent(child, &actual_parent);
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
      ast_node_get_parent(child, &actual_parent);
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
      ast_node_get_parent(child, &actual_parent);
      asserteq_ptr(actual_parent, parent);
    }

    it("Should reject a NULL insert parent") {
      // Arrange: prepare resources for the function.
      AstNode* child = ast_test_node(ctx);

      // Act: execute the function.
      assertThrow(E_INTERNAL_ERROR,
                  { ast_node_insert_child(ctx, NULL, 0, child); });

      // Assert: check the observable result.
      AstNode* actual = child;
      ast_node_get_parent(child, &actual);
      asserteq_ptr(actual, NULL);
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
      ast_node_get_parent(child, &actual);
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
        ast_node_get_parent(actual, &actual_parent);
        asserteq_ptr(actual_parent, parent);
      }
      AstNode* actual_parent = parent;
      ast_node_get_parent(detached, &actual_parent);
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
        ast_node_get_parent(actual, &actual_parent);
        asserteq_ptr(actual_parent, parent);
      }
      AstNode* actual_parent = parent;
      ast_node_get_parent(detached, &actual_parent);
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
        ast_node_get_parent(actual, &actual_parent);
        asserteq_ptr(actual_parent, parent);
      }
      AstNode* actual_parent = parent;
      ast_node_get_parent(detached, &actual_parent);
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
        ast_node_get_parent(actual, &actual_parent);
        asserteq_ptr(actual_parent, parent);
      }
      AstNode* actual_parent = parent;
      ast_node_get_parent(detached, &actual_parent);
      asserteq_ptr(actual_parent, NULL);
      ast_node_set_type(detached, AST_NODE_RETURN);
      asserteq(ast_node_get_type(detached), AST_NODE_RETURN);
    }

    it("Should reject a NULL detach parent") {
      // Arrange: prepare resources for the function.
      AstNode* detached = NULL;

      // Act: execute the function.
      assertThrow(E_INTERNAL_ERROR,
                  { ast_node_detach_child(ctx, NULL, 0, &detached); });

      // Assert: check the observable result.
      asserteq(ctx->exit_code, E_INTERNAL_ERROR);
    }

    it("Should reject detachment from an empty node") {
      // Arrange: prepare resources for the function.
      AstNode* parent = ast_test_node(ctx);
      AstNode* detached = NULL;

      // Act: execute the function.
      assertThrow(E_INTERNAL_ERROR,
                  { ast_node_detach_child(ctx, parent, 0, &detached); });

      // Assert: check the observable result.
      asserteq(ast_node_get_child_count(parent), 0);
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

snow_main();
