#include "../includes/fs_core.h"
#include <stdio.h>
#include <string.h>

static uint8_t disk1[TOTAL_BLOCKS][BLOCK_SIZE];
static uint8_t disk2[TOTAL_BLOCKS][BLOCK_SIZE];
static uint32_t block_checksums[TOTAL_BLOCKS];

static int disk1_online = 1;
static int disk2_online = 1;
static int self_heal_count = 0;

static int block_used[TOTAL_BLOCKS];
static Inode inode_table[MAX_FILES];

//Done
uint32_t calculate_checksum(const uint8_t *data, int size) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < size; i++) {
        hash = (hash ^ data[i]) * 16777619u;
    }
    return hash;
}

