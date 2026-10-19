/**
 * @file bst.c
 * Project: IFJ26
 *
 * @author David Daniel Slapal (xslapad00)
 * @date 9. 10. 2026
 *
 * @brief Height-balanced search tree.
 */

#include "bst.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Calculate maximum of two values
 * @param a First value
 * @param b Second value
 * @return Largest of the two values
 */
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/**
 * @brief Initialize a new BST node
 * @param node Pointer to the node to initialize
 * @param key Lookup key for the node in the BST
 * @param data Data payload of the node
 */
static void bst_node_init(BstNode* node, char* key, void* data) {
  node->balance = 0;
  node->height = 1;
  node->child_left = NULL;
  node->child_right = NULL;
  node->key = key;
  node->data = data;
}

/**
 * @brief Get BST node height
 * @param node Pointer to the node
 * @return Height of the subtree rooted at the node, or 0 if NULL
 */
static int bst_node_get_height(BstNode* node) {
  if (node == NULL)
    return 0;
  return node->height;
}

/**
 * @brief Calculate height of the BST node and store it
 * @param node Pointer to the node
 */
static void bst_node_recalculate_height(BstNode* node) {
  if (node == NULL)
    return;
  node->height = 1 + MAX(bst_node_get_height(node->child_left),
                         bst_node_get_height(node->child_right));
}

/**
 * @brief Get BST node balance
 * @param node Pointer to the node
 * @return Balance factor, or 0 if NULL
 */
static int bst_node_get_balance(BstNode* node) {
  if (node == NULL)
    return 0;
  return node->balance;
}

/**
 * @brief Calculate balance of the BST node and store it
 * @param node Pointer to the node
 */
static void bst_node_recalculate_balance(BstNode* node) {
  if (node == NULL)
    return;
  node->balance = bst_node_get_height(node->child_right) -
                  bst_node_get_height(node->child_left);
}

/**
 * @brief Recalculate height and balance of the node
 * @param node Pointer to the node
 */
static void bst_node_update_metadata(BstNode* node) {
  bst_node_recalculate_height(node);
  bst_node_recalculate_balance(node);
}

/**
 * @brief Rotate critical node's subtree left.
 * @param node Pointer to the critical node
 * @return Pointer to the root of rotated subtree
 */
static BstNode* bst_rotate_left(BstNode* node) {
  BstNode* x = node->child_right;
  node->child_right = x->child_left;
  x->child_left = node;

  bst_node_update_metadata(node);
  bst_node_update_metadata(x);
  return x;
}

/**
 * @brief Rotate critical node's subtree right.
 * @param node Pointer to the critical node
 * @return Pointer to the root of rotated subtree
 */
static BstNode* bst_rotate_right(BstNode* node) {
  BstNode* x = node->child_left;
  node->child_left = x->child_right;
  x->child_right = node;

  bst_node_update_metadata(node);
  bst_node_update_metadata(x);
  return x;
}

/**
 * @brief Check balance around critical node and if unbalanced, perform
 * rotations
 * @param node Pointer to the critical node
 * @return Pointer to the modified critical node
 */
static BstNode* bst_node_repair_balance(BstNode* node) {
  if (bst_node_get_balance(node) == -2) {
    // LL
    if (bst_node_get_balance(node->child_left) <= 0) {
      node = bst_rotate_right(node);
    }
    // LR
    else if (bst_node_get_balance(node->child_left) == 1) {
      node->child_left = bst_rotate_left(node->child_left);
      node = bst_rotate_right(node);
    }
  } else if (bst_node_get_balance(node) == 2) {
    // RR
    if (bst_node_get_balance(node->child_right) >= 0) {
      node = bst_rotate_left(node);
    }
    // RL
    else if (bst_node_get_balance(node->child_right) == -1) {
      node->child_right = bst_rotate_right(node->child_right);
      node = bst_rotate_left(node);
    }
  }
  return node;
}

/**
 * @brief Destructs a BST subtree defined by root node
 * @param node Pointer to the root node
 */
static void bst_node_destroy_subtree(BstNode* node) {
  if (node == NULL)
    return;
  bst_node_destroy_subtree(node->child_left);
  bst_node_destroy_subtree(node->child_right);
  if (node->data != NULL)
    free(node->data);
  if (node->key != NULL)
    free(node->key);
  free(node);
}

/**
 * @brief Move data from one BST node to another and destroy the old one
 * @param target_node Pointer to the target node
 * @param origin_node Pointer to the origin node to be destroyed
 */
static void bst_node_move_data(BstNode* target_node, BstNode* origin_node) {
  if (target_node->data != NULL && target_node->data != origin_node->data)
    free(target_node->data);
  target_node->data = origin_node->data;
  if (origin_node->key != NULL)
    free(origin_node->key);
  free(origin_node);
}

/**
 * @brief Helper for recursive BST node insertion
 * @param tree_root Pointer to root node of the subtree
 * @param new_node Pointer to node to be inserted into the subtree
 * @return Modified subtree root
 */
static BstNode* bst_subtree_insert_node(BstNode* tree_root, BstNode* new_node) {
  if (tree_root == NULL)
    return new_node;

  if (strcmp(new_node->key, tree_root->key) < 0) {
    tree_root->child_left =
        bst_subtree_insert_node(tree_root->child_left, new_node);
  } else if (strcmp(new_node->key, tree_root->key) > 0) {
    tree_root->child_right =
        bst_subtree_insert_node(tree_root->child_right, new_node);
  } else if (strcmp(new_node->key, tree_root->key) == 0) {
    bst_node_move_data(tree_root, new_node);
  }

  bst_node_update_metadata(tree_root);
  tree_root = bst_node_repair_balance(tree_root);

  return tree_root;
}

/**
 * @brief Initialize a new BST
 * @param tree Pointer to the tree to initialize
 * @pre Tree must be allocated
 */
void bst_init(Bst* tree) {
  tree->root = NULL;
}

/**
 * @brief Search data in BST by key
 * @param tree Pointer to the tree to search in
 * @param key Key to search for in the tree
 * @return Pointer to the data if found, or NULL
 */
void* bst_search(Bst* tree, const char* key) {
  if (tree == NULL || key == NULL)
    return NULL;

  BstNode* current_node = tree->root;
  while (current_node != NULL) {
    int comparison = strcmp(key, current_node->key);
    if (comparison == 0)
      return current_node->data;
    else if (comparison < 0)
      current_node = current_node->child_left;
    else if (comparison > 0)
      current_node = current_node->child_right;
  }
  return NULL;
}

/**
 * @brief Configure node and insert it into BST
 * @param ctx Error context
 * @param tree Pointer to the tree
 * @param key Key of the node
 * @param data Data stored in the node
 * @pre Key is allocated on stack. If on heap, caller must free.
 * @pre Data are allocated on heap.
 */
void bst_insert(Ctx* ctx, Bst* tree, const char* key, void* data) {
  if (tree == NULL || key == NULL)
    return;

  BstNode* node = TRY_MALLOC(sizeof(BstNode));
  char* node_key = TRY_MALLOC(strlen(key) + 1);
  strcpy(node_key, key);
  bst_node_init(node, node_key, data);

  tree->root = bst_subtree_insert_node(tree->root, node);
}

/**
 * @brief Destructor for BST
 * @param tree Pointer to the tree to destroy
 */
void bst_destroy(Bst* tree) {
  if (tree == NULL)
    return;
  bst_node_destroy_subtree(tree->root);
  tree->root = NULL;
}
