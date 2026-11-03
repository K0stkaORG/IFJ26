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
static void bst_node_init(BstNode* node, void* key, void* data) {
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
 * @param key_destructor Function that destructs key
 * @param data_destructor Function that destructs data
 */
static void bst_node_destroy_subtree(BstNode* node,
                                     BstKeyDestructor key_destructor,
                                     BstDataDestructor data_destructor) {
  if (node == NULL)
    return;
  bst_node_destroy_subtree(node->child_left, key_destructor, data_destructor);
  bst_node_destroy_subtree(node->child_right, key_destructor, data_destructor);
  if (node->data != NULL && data_destructor != NULL)
    data_destructor(node->data);
  if (node->key != NULL && key_destructor != NULL)
    key_destructor(node->key);
  free(node);
}

/**
 * @brief Move data from one BST node to another and destroy the old one
 * @param target_node Pointer to the target node
 * @param origin_node Pointer to the origin node to be destroyed
 * @param key_destructor Function that destructs key
 * @param data_destructor Function that destructs data
 */
static void bst_node_move_data(BstNode* target_node,
                               BstNode* origin_node,
                               BstKeyDestructor key_destructor,
                               BstDataDestructor data_destructor) {
  if (target_node->data != NULL && target_node->data != origin_node->data &&
      data_destructor != NULL)
    data_destructor(target_node->data);
  target_node->data = origin_node->data;
  if (origin_node->key != NULL && target_node->key != origin_node->key &&
      key_destructor != NULL)
    key_destructor(origin_node->key);
  free(origin_node);
}

/**
 * @brief Helper for recursive BST node insertion
 * @param tree_root Pointer to root node of the subtree
 * @param new_node Pointer to node to be inserted into the subtree
 * @param key_comparator Function compares two keys
 * @param key_destructor Function that destructs key
 * @param data_destructor Function that destructs data
 * @return Modified subtree root
 */
static BstNode* bst_subtree_insert_node(BstNode* tree_root,
                                        BstNode* new_node,
                                        BstKeyComparator key_comparator,
                                        BstKeyDestructor key_destructor,
                                        BstDataDestructor data_destructor) {
  if (tree_root == NULL)
    return new_node;

  if (key_comparator(new_node->key, tree_root->key) < 0) {
    tree_root->child_left =
        bst_subtree_insert_node(tree_root->child_left, new_node, key_comparator,
                                key_destructor, data_destructor);
  } else if (key_comparator(new_node->key, tree_root->key) > 0) {
    tree_root->child_right = bst_subtree_insert_node(
        tree_root->child_right, new_node, key_comparator, key_destructor,
        data_destructor);
  } else if (key_comparator(new_node->key, tree_root->key) == 0) {
    bst_node_move_data(tree_root, new_node, key_destructor, data_destructor);
  }

  bst_node_update_metadata(tree_root);
  tree_root = bst_node_repair_balance(tree_root);

  return tree_root;
}

/**
 * @brief Initialize a new BST
 * @param tree Pointer to the tree to initialize
 * @param key_comparator Function that compares abstract keys
 * @param key_destructor Function that destructs key, or NULL if not owned
 * @param data_destructor Function that destructs data, or NULL if not owned
 * @pre Tree must be allocated
 * @pre key_comparator must not be NULL and must be a function that returns int
 */
void bst_init(Bst* tree,
              BstKeyComparator key_comparator,
              BstKeyDestructor key_destructor,
              BstDataDestructor data_destructor) {
  tree->root = NULL;
  tree->key_comparator = key_comparator;
  tree->key_destructor = key_destructor;
  tree->data_destructor = data_destructor;
}

/**
 * @brief Search data in BST by key
 * @param tree Pointer to the tree to search in
 * @param key Key to search for in the tree
 * @return Pointer to the data if found, or NULL
 */
void* bst_search(Bst* tree, void* key) {
  if (tree == NULL || key == NULL)
    return NULL;

  BstNode* current_node = tree->root;
  while (current_node != NULL) {
    int comparison = tree->key_comparator(key, current_node->key);
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
 * @pre Tree is initialized
 * @pre If key destructor is set, key must be allocated on heap.
 * @pre If data destructor is set, data must be allocated on heap.
 * @pre Data must not be reinserted under different key.
 */
void bst_insert(Ctx* ctx, Bst* tree, void* key, void* data) {
  if (key == NULL)
    return;

  BstNode* node = TRY_MALLOC(sizeof(BstNode));
  bst_node_init(node, key, data);

  tree->root =
      bst_subtree_insert_node(tree->root, node, tree->key_comparator,
                              tree->key_destructor, tree->data_destructor);
}

/**
 * @brief Destructor for BST
 * @param tree Pointer to the tree to destroy
 */
void bst_destroy(Bst* tree) {
  if (tree == NULL)
    return;
  bst_node_destroy_subtree(tree->root, tree->key_destructor,
                           tree->data_destructor);
  tree->root = NULL;
}
