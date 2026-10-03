#include "../includes/btree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static BTreeNode *btree_root = NULL;

static BTreeNode* btree_create_node(int is_leaf) {
    BTreeNode *node = (BTreeNode *)malloc(sizeof(BTreeNode));
    if (node == NULL) {
        fprintf(stderr, "[BTREE ERROR] Memory allocation failed!\n");
        return NULL;
    }
    node->num_keys = 0;
    node->is_leaf = is_leaf;
    for (int i = 0; i < 2 * BTREE_T; i++) {
        node->children[i] = NULL;
    }
    return node;
}

void btree_init(void) {
    btree_destroy();
    btree_root = NULL;
}

static int btree_search_node(BTreeNode *node, const char *filename) {
    if (node == NULL || filename == NULL) {
        return -1;
    }

    int i = 0;
    while (i < node->num_keys && strcmp(filename, node->entries[i].filename) > 0) {
        i++;
    }

    if (i < node->num_keys && strcmp(filename, node->entries[i].filename) == 0) {
        return node->entries[i].inode_id;
    }

    if (node->is_leaf) {
        return -1;
    }

    return btree_search_node(node->children[i], filename);
}

int btree_search(const char *filename) {
    return btree_search_node(btree_root, filename);
}

static void btree_split_child(BTreeNode *parent, int i, BTreeNode *child) {
    BTreeNode *z = btree_create_node(child->is_leaf);
    z->num_keys = BTREE_T - 1;

    for (int j = 0; j < BTREE_T - 1; j++) {
        z->entries[j] = child->entries[j + BTREE_T];
    }

    if (!child->is_leaf) {
        for (int j = 0; j < BTREE_T; j++) {
            z->children[j] = child->children[j + BTREE_T];
        }
    }

    child->num_keys = BTREE_T - 1;

    for (int j = parent->num_keys; j >= i + 1; j--) {
        parent->children[j + 1] = parent->children[j];
    }
    parent->children[i + 1] = z;

    for (int j = parent->num_keys - 1; j >= i; j--) {
        parent->entries[j + 1] = parent->entries[j];
    }
    parent->entries[i] = child->entries[BTREE_T - 1];
    parent->num_keys++;
}

static void btree_insert_non_full(BTreeNode *node, const char *filename, int inode_id) {
    int i = node->num_keys - 1;

    if (node->is_leaf) {
        while (i >= 0 && strcmp(filename, node->entries[i].filename) < 0) {
            node->entries[i + 1] = node->entries[i];
            i--;
        }

        strncpy(node->entries[i + 1].filename, filename, MAX_FILENAME - 1);
        node->entries[i + 1].filename[MAX_FILENAME - 1] = '\0';
        node->entries[i + 1].inode_id = inode_id;
        node->num_keys++;
    } else {
        while (i >= 0 && strcmp(filename, node->entries[i].filename) < 0) {
            i--;
        }
        i++;

        if (node->children[i]->num_keys == 2 * BTREE_T - 1) {
            btree_split_child(node, i, node->children[i]);
            if (strcmp(filename, node->entries[i].filename) > 0) {
                i++;
            }
        }
        btree_insert_non_full(node->children[i], filename, inode_id);
    }
}

void btree_insert(const char *filename, int inode_id) {
    if (filename == NULL) return;

    if (btree_root == NULL) {
        btree_root = btree_create_node(1);
        strncpy(btree_root->entries[0].filename, filename, MAX_FILENAME - 1);
        btree_root->entries[0].filename[MAX_FILENAME - 1] = '\0';
        btree_root->entries[0].inode_id = inode_id;
        btree_root->num_keys = 1;
        return;
    }

    if (btree_root->num_keys == 2 * BTREE_T - 1) {
        BTreeNode *s = btree_create_node(0);
        s->children[0] = btree_root;
        btree_split_child(s, 0, btree_root);

        int i = 0;
        if (strcmp(filename, s->entries[0].filename) > 0) {
            i++;
        }
        btree_insert_non_full(s->children[i], filename, inode_id);
        btree_root = s;
    } else {
        btree_insert_non_full(btree_root, filename, inode_id);
    }
}

static int find_key_index(BTreeNode *node, const char *filename) {
    int idx = 0;
    while (idx < node->num_keys && strcmp(node->entries[idx].filename, filename) < 0) {
        idx++;
    }
    return idx;
}

static void btree_borrow_from_prev(BTreeNode *node, int idx) {
    BTreeNode *child = node->children[idx];
    BTreeNode *sibling = node->children[idx - 1];

    for (int i = child->num_keys - 1; i >= 0; i--) {
        child->entries[i + 1] = child->entries[i];
    }

    if (!child->is_leaf) {
        for (int i = child->num_keys; i >= 0; i--) {
            child->children[i + 1] = child->children[i];
        }
    }

    child->entries[0] = node->entries[idx - 1];
    if (!child->is_leaf) {
        child->children[0] = sibling->children[sibling->num_keys];
    }

    node->entries[idx - 1] = sibling->entries[sibling->num_keys - 1];
    child->num_keys++;
    sibling->num_keys--;
}

static void btree_borrow_from_next(BTreeNode *node, int idx) {
    BTreeNode *child = node->children[idx];
    BTreeNode *sibling = node->children[idx + 1];

    child->entries[child->num_keys] = node->entries[idx];
    if (!child->is_leaf) {
        child->children[child->num_keys + 1] = sibling->children[0];
    }

    node->entries[idx] = sibling->entries[0];

    for (int i = 1; i < sibling->num_keys; i++) {
        sibling->entries[i - 1] = sibling->entries[i];
    }

    if (!sibling->is_leaf) {
        for (int i = 1; i <= sibling->num_keys; i++) {
            sibling->children[i - 1] = sibling->children[i];
        }
    }

    child->num_keys++;
    sibling->num_keys--;
}

static void btree_merge_children(BTreeNode *node, int idx) {
    BTreeNode *child = node->children[idx];
    BTreeNode *sibling = node->children[idx + 1];

    child->entries[BTREE_T - 1] = node->entries[idx];

    for (int i = 0; i < sibling->num_keys; i++) {
        child->entries[i + BTREE_T] = sibling->entries[i];
    }

    if (!child->is_leaf) {
        for (int i = 0; i <= sibling->num_keys; i++) {
            child->children[i + BTREE_T] = sibling->children[i];
        }
    }

    for (int i = idx + 1; i < node->num_keys; i++) {
        node->entries[i - 1] = node->entries[i];
    }

    for (int i = idx + 2; i <= node->num_keys; i++) {
        node->children[i - 1] = node->children[i];
    }

    child->num_keys += sibling->num_keys + 1;
    node->num_keys--;
    free(sibling);
}

static BTreeEntry btree_get_predecessor(BTreeNode *node, int idx) {
    BTreeNode *cur = node->children[idx];
    while (!cur->is_leaf) {
        cur = cur->children[cur->num_keys];
    }
    return cur->entries[cur->num_keys - 1];
}

static BTreeEntry btree_get_successor(BTreeNode *node, int idx) {
    BTreeNode *cur = node->children[idx + 1];
    while (!cur->is_leaf) {
        cur = cur->children[0];
    }
    return cur->entries[0];
}

static void btree_delete_from_node(BTreeNode *node, const char *filename) {
    int idx = find_key_index(node, filename);

    if (idx < node->num_keys && strcmp(node->entries[idx].filename, filename) == 0) {
        if (node->is_leaf) {
            for (int i = idx + 1; i < node->num_keys; i++) {
                node->entries[i - 1] = node->entries[i];
            }
            node->num_keys--;
        } else {
            if (node->children[idx]->num_keys >= BTREE_T) {
                BTreeEntry pred = btree_get_predecessor(node, idx);
                node->entries[idx] = pred;
                btree_delete_from_node(node->children[idx], pred.filename);
            } else if (node->children[idx + 1]->num_keys >= BTREE_T) {
                BTreeEntry succ = btree_get_successor(node, idx);
                node->entries[idx] = succ;
                btree_delete_from_node(node->children[idx + 1], succ.filename);
            } else {
                btree_merge_children(node, idx);
                btree_delete_from_node(node->children[idx], filename);
            }
        }
    } else {
        if (node->is_leaf) {
            return;
        }

        int flag = (idx == node->num_keys) ? 1 : 0;

        if (node->children[idx]->num_keys < BTREE_T) {
            if (idx != 0 && node->children[idx - 1]->num_keys >= BTREE_T) {
                btree_borrow_from_prev(node, idx);
            } else if (idx != node->num_keys && node->children[idx + 1]->num_keys >= BTREE_T) {
                btree_borrow_from_next(node, idx);
            } else {
                if (idx != node->num_keys) {
                    btree_merge_children(node, idx);
                } else {
                    btree_merge_children(node, idx - 1);
                }
            }
        }

        if (flag && idx > node->num_keys) {
            btree_delete_from_node(node->children[idx - 1], filename);
        } else {
            btree_delete_from_node(node->children[idx], filename);
        }
    }
}

void btree_delete(const char *filename) {
    if (btree_root == NULL || filename == NULL) return;

    btree_delete_from_node(btree_root, filename);

    if (btree_root->num_keys == 0) {
        BTreeNode *tmp = btree_root;
        if (btree_root->is_leaf) {
            btree_root = NULL;
        } else {
            btree_root = btree_root->children[0];
        }
        free(tmp);
    }
}

static void btree_inorder_node(BTreeNode *node, void (*callback)(const char *filename, int inode_id)) {
    if (node == NULL || callback == NULL) return;

    int i = 0;
    for (i = 0; i < node->num_keys; i++) {
        if (!node->is_leaf) {
            btree_inorder_node(node->children[i], callback);
        }
        callback(node->entries[i].filename, node->entries[i].inode_id);
    }

    if (!node->is_leaf) {
        btree_inorder_node(node->children[i], callback);
    }
}

void btree_inorder(void (*callback)(const char *filename, int inode_id)) {
    btree_inorder_node(btree_root, callback);
}

static void btree_print_node(BTreeNode *node, int level) {
    if (node == NULL) return;

    for (int i = 0; i < level; i++) {
        printf("    ");
    }
    printf("[%s] (%d keys): ", node->is_leaf ? "Leaf" : "Internal", node->num_keys);
    for (int i = 0; i < node->num_keys; i++) {
        printf("'%s' (id:%d)%s", node->entries[i].filename, node->entries[i].inode_id,
               (i == node->num_keys - 1) ? "" : ", ");
    }
    printf("\n");

    if (!node->is_leaf) {
        for (int i = 0; i <= node->num_keys; i++) {
            btree_print_node(node->children[i], level + 1);
        }
    }
}

void btree_print(void) {
    if (btree_root == NULL) {
        printf("(B-Tree is empty)\n");
        return;
    }
    printf("\n============= B-TREE STRUCTURE (Order T=%d) =============\n", BTREE_T);
    btree_print_node(btree_root, 0);
    printf("=========================================================\n\n");
}

static void btree_destroy_node(BTreeNode *node) {
    if (node == NULL) return;
    if (!node->is_leaf) {
        for (int i = 0; i <= node->num_keys; i++) {
            btree_destroy_node(node->children[i]);
        }
    }
    free(node);
}

void btree_destroy(void) {
    btree_destroy_node(btree_root);
    btree_root = NULL;
}
