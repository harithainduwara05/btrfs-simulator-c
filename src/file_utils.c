#include "../includes/file_utils.h"
#include <stdio.h>
#include <string.h>

int fs_copy_file(const char *src_filename, const char *dest_filename) {
    if (src_filename == NULL || dest_filename == NULL) {
        printf("[FILE-UTILS ERROR] Invalid filename.\n");
        return -1;
    }

    if (strlen(dest_filename) >= MAX_FILENAME) {
        printf("[FILE-UTILS ERROR] Destination filename '%s' is too long.\n", dest_filename);
        return -1;
    }

    int src_id = find_inode_by_name(src_filename);
    if (src_id == -1) {
        printf("[FILE-UTILS ERROR] Source file '%s' not found.\n", src_filename);
        return -1;
    }

    if (find_inode_by_name(dest_filename) != -1) {
        printf("[FILE-UTILS ERROR] File '%s' already exists.\n", dest_filename);
        return -1;
    }

    int dest_id = allocate_inode();
    if (dest_id == -1) {
        printf("[FILE-UTILS ERROR] No free inodes left to copy '%s'.\n", dest_filename);
        return -1;
    }

    Inode *src = get_inode(src_id);
    Inode *dest = get_inode(dest_id);

    strncpy(dest->filename, dest_filename, MAX_FILENAME - 1);
    dest->filename[MAX_FILENAME - 1] = '\0';
    dest->size = src->size;
    dest->block_count = src->block_count;

    for (int i = 0; i < src->block_count; i++) {
        dest->block_pointers[i] = src->block_pointers[i];
    }

    src->ref_count++;
    dest->ref_count = src->ref_count;

    printf("[FILE-UTILS CoW] Successfully copied '%s' to '%s' (shared %d blocks, 0 extra disk blocks allocated).\n",
           src_filename, dest_filename, dest->block_count);
    return dest_id;
}

int fs_delete_file(const char *filename) {
    if (filename == NULL) {
        printf("[FILE-UTILS ERROR] Invalid filename.\n");
        return -1;
    }

    int inode_id = find_inode_by_name(filename);
    if (inode_id == -1) {
        printf("[FILE-UTILS ERROR] File '%s' not found.\n", filename);
        return -1;
    }

    Inode *target = get_inode(inode_id);

    for (int i = 0; i < target->block_count; i++) {
        int blk = target->block_pointers[i];
        int is_shared = 0;

        for (int j = 0; j < MAX_FILES; j++) {
            if (j == inode_id) continue;
            Inode *other = get_inode(j);
            if (other != NULL) {
                for (int k = 0; k < other->block_count; k++) {
                    if (other->block_pointers[k] == blk) {
                        is_shared = 1;
                        if (other->ref_count > 1) {
                            other->ref_count--;
                        }
                        break;
                    }
                }
            }
            if (is_shared) break;
        }

        if (!is_shared) {
            free_block(blk);
        }
    }

    free_inode(inode_id);
    printf("[FILE-UTILS] Successfully deleted '%s'.\n", filename);
    return 0;
}

int fs_read_file(const char *filename, void *buffer, int max_len) {
    if (filename == NULL || buffer == NULL || max_len <= 0) {
        return -1;
    }

    int inode_id = find_inode_by_name(filename);
    if (inode_id == -1) {
        printf("[FILE-UTILS ERROR] File '%s' not found.\n", filename);
        return -1;
    }

    Inode *inode = get_inode(inode_id);
    uint8_t *dest = (uint8_t *)buffer;
    int bytes_read = 0;

    for (int i = 0; i < inode->block_count; i++) {
        uint8_t block_buf[BLOCK_SIZE];
        if (raid1_read_block(inode->block_pointers[i], block_buf) != 0) {
            printf("[FILE-UTILS ERROR] Failed to read block %d for '%s'.\n",
                   inode->block_pointers[i], filename);
            return -1;
        }

        int remaining = inode->size - bytes_read;
        int to_copy = (remaining > BLOCK_SIZE) ? BLOCK_SIZE : remaining;
        if (bytes_read + to_copy > max_len) {
            to_copy = max_len - bytes_read;
        }

        if (to_copy > 0) {
            memcpy(dest + bytes_read, block_buf, to_copy);
            bytes_read += to_copy;
        }

        if (bytes_read >= inode->size || bytes_read >= max_len) {
            break;
        }
    }

    if (bytes_read < max_len) {
        dest[bytes_read] = '\0';
    }

    return bytes_read;
}
