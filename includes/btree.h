#ifndef BTREE_H
#define BTREE_H

#include "fs_core.h"

#define BTREE_T 2

typedef struct {
    char filename[MAX_FILENAME];
    int inode_id;
} BTreeEntry;

typedef struct BTreeNode {
    int num_keys;
    int is_leaf;
    BTreeEntry entries[2 * BTREE_T - 1];
    struct BTreeNode *children[2 * BTREE_T];
} BTreeNode;

void btree_init(void);
int  btree_search(const char *filename);
void btree_insert(const char *filename, int inode_id);
void btree_delete(const char *filename);
void btree_inorder(void (*callback)(const char *filename, int inode_id));
void btree_print(void);
void btree_destroy(void);

#endif
