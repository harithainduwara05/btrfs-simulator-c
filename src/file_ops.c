#include "../includes/file_ops.h"
#include <stdio.h>
#include <string.h>

int fs_create_file(const char *filename, const void *data, int size) {
    if (filename == NULL || size < 0) {
        printf("[FILE-OPS ERROR] Invalid filename or size.\n");
        return -1;
    }

    if (strlen(filename) >= MAX_FILENAME) {
        printf("[FILE-OPS ERROR] Filename '%s' is too long (max %d chars).\n",
               filename, MAX_FILENAME - 1);
        return -1;
    }

    if (find_inode_by_name(filename) != -1) {
        printf("[FILE-OPS ERROR] A file named '%s' already exists.\n", filename);
        return -1;
    }

    int blocks_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (blocks_needed == 0) {
        blocks_needed = 1; /* still reserve one block for an empty file */
    }
    if (blocks_needed > MAX_BLOCKS_PER_FILE) {
        printf("[FILE-OPS ERROR] File too large: needs %d blocks, max is %d.\n",
               blocks_needed, MAX_BLOCKS_PER_FILE);
        return -1;
    }

    int inode_id = allocate_inode();
    if (inode_id == -1) {
        printf("[FILE-OPS ERROR] No free inodes left. Cannot create '%s'.\n", filename);
        return -1;
    }

    Inode *inode = get_inode(inode_id);
    strncpy(inode->filename, filename, MAX_FILENAME - 1);
    inode->filename[MAX_FILENAME - 1] = '\0';
    inode->size = size;
    inode->block_count = 0;

    const uint8_t *src = (const uint8_t *)data;

    for (int i = 0; i < blocks_needed; i++) {
        int block_num = allocate_block();
        if (block_num == -1) {
            printf("[FILE-OPS ERROR] Ran out of free blocks while writing '%s'.\n", filename);
            /* Roll back: release any blocks already allocated for this file */
            for (int j = 0; j < inode->block_count; j++) {
                free_block(inode->block_pointers[j]);
            }
            free_inode(inode_id);
            return -1;
        }

        uint8_t buffer[BLOCK_SIZE] = {0};
        int offset = i * BLOCK_SIZE;
        int remaining = size - offset;
        int chunk = (remaining > BLOCK_SIZE) ? BLOCK_SIZE : remaining;
        if (chunk > 0 && data != NULL) {
            memcpy(buffer, src + offset, chunk);
        }

        if (raid1_write_block(block_num, buffer) != 0) {
            printf("[FILE-OPS ERROR] RAID-1 write failed for block %d.\n", block_num);
            free_block(block_num);
            for (int j = 0; j < inode->block_count; j++) {
                free_block(inode->block_pointers[j]);
            }
            free_inode(inode_id);
            return -1;
        }

        inode->block_pointers[inode->block_count] = block_num;
        inode->block_count++;
    }

    printf("[FILE-OPS] Created '%s' (%d bytes, %d block%s, inode %d).\n",
           filename, size, inode->block_count,
           inode->block_count == 1 ? "" : "s", inode_id);

    return inode_id;
}

void fs_list_files(void) {
    int found = 0;

    printf("\n--------------------------- FILE LIST ---------------------------\n");
    printf(" %-3s %-30s %10s %8s\n", "ID", "Filename", "Size(B)", "Blocks");
    printf("-------------------------------------------------------------------\n");

    for (int i = 0; i < MAX_FILES; i++) {
        Inode *inode = get_inode(i);
        if (inode != NULL) {
            printf(" %-3d %-30s %10d %8d\n",
                   i, inode->filename, inode->size, inode->block_count);
            found++;
        }
    }

    if (found == 0) {
        printf("               (no files found on the filesystem)\n");
    }

    printf("-------------------------------------------------------------------\n");
    printf(" Total files: %d / %d\n\n", found, MAX_FILES);
}