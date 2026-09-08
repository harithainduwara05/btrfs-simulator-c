#ifndef FS_CORE_H
#define FS_CORE_H

#include <stdint.h>
#include <stddef.h>

#define BLOCK_SIZE 128
#define TOTAL_BLOCKS 1000
#define MAX_FILES 64
#define MAX_FILENAME 128
#define MAX_BLOCKS_PER_FILE 20

typedef struct {
    char filename[MAX_FILENAME];
    int size;
    int block_pointers[MAX_BLOCKS_PER_FILE];
    int block_count;
    int is_used;
    int ref_count;
} Inode;

void fs_init(void);

int raid1_write_block(int block_num, const void *buffer);
int raid1_read_block(int block_num, void *buffer);
void raid1_corrupt_block(int disk_id, int block_num);
void raid1_set_disk_status(int disk_id, int is_online);
void raid1_print_status(void);

int allocate_block(void);
void free_block(int block_num);
void inc_block_ref(int block_num);
void dec_block_ref(int block_num);
int get_block_ref(int block_num);
int allocate_inode(void);
void free_inode(int inode_id);
Inode* get_inode(int inode_id);
int find_inode_by_name(const char *name);

uint32_t calculate_checksum(const uint8_t *data, int size);

#endif