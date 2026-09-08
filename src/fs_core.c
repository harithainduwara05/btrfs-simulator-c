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
//done RAID1 WRITE
int raid1_write_block(int block_num, const void *buffer) {
    if (block_num < 0 || block_num >= TOTAL_BLOCKS || buffer == NULL) {
        return -1;
    }
    if (!disk1_online && !disk2_online) {
        printf("[RAID-1 ERROR] Write failed: Both disks are offline!\n");
        return -1;
    }

    block_checksums[block_num] = calculate_checksum((const uint8_t *)buffer, BLOCK_SIZE);

    if (disk1_online) {
        memcpy(disk1[block_num], buffer, BLOCK_SIZE);
    }
    if (disk2_online) {
        memcpy(disk2[block_num], buffer, BLOCK_SIZE);
    }

    return 0;
}
//DONE RAID1 READ
int raid1_read_block(int block_num, void *buffer) {
    if (block_num < 0 || block_num >= TOTAL_BLOCKS || buffer == NULL) {
        return -1;
    }

    uint32_t expected_checksum = block_checksums[block_num];

    if (disk1_online) {
        uint32_t csum1 = calculate_checksum(disk1[block_num], BLOCK_SIZE);

        if (csum1 == expected_checksum) {
            memcpy(buffer, disk1[block_num], BLOCK_SIZE);
            return 0;
        }

        printf("[BTRFS ALERT] Data corruption detected on Disk 1 at Block %d!\n", block_num);
    }

    if (disk2_online) {
        uint32_t csum2 = calculate_checksum(disk2[block_num], BLOCK_SIZE);

        if (csum2 == expected_checksum) {
            memcpy(buffer, disk2[block_num], BLOCK_SIZE);

            if (disk1_online) {
                memcpy(disk1[block_num], disk2[block_num], BLOCK_SIZE);
                self_heal_count++;
                printf("[BTRFS SELF-HEAL] Repaired Disk 1 Block %d using healthy data from Disk 2!\n", block_num);
            }
            return 0;
        }

        printf("[BTRFS ERROR] Data corruption detected on Disk 2 at Block %d!\n", block_num);
    }

    printf("[BTRFS CRITICAL] Read failed! Both disks are unavailable or corrupted at Block %d.\n", block_num);
    return -1;
}
//Test the RAID 1 Self Healing Process
void raid1_corrupt_block(int disk_id, int block_num) {
    if (block_num < 0 || block_num >= TOTAL_BLOCKS) return;

    if (disk_id == 1) {
        disk1[block_num][0] ^= 0xFF;
        printf("[SIMULATION] Corrupted Block %d on Disk 1.\n", block_num);
    } else if (disk_id == 2) {
        disk2[block_num][0] ^= 0xFF;
        printf("[SIMULATION] Corrupted Block %d on Disk 2.\n", block_num);
    }
}

//Test the RAID 1 Disk Status Changes
void raid1_set_disk_status(int disk_id, int is_online) {
    if (disk_id == 1) {
        disk1_online = is_online;
        printf("[SIMULATION] Disk 1 is now %s.\n", is_online ? "ONLINE" : "OFFLINE");
    } else if (disk_id == 2) {
        disk2_online = is_online;
        printf("[SIMULATION] Disk 2 is now %s.\n", is_online ? "ONLINE" : "OFFLINE");
    }
}

void raid1_print_status(void) {
    printf("\n---------------------- RAID-1 STATUS ---------------------\n");
    printf(" Disk 1 Status         : %s\n", disk1_online ? "[ONLINE]" : "[OFFLINE / FAILED]");
    printf(" Disk 2 Status         : %s\n", disk2_online ? "[ONLINE]" : "[OFFLINE / FAILED]");
    printf(" Self-Heal Operations  : %d\n", self_heal_count);
    printf("-----------------------------------------------------------\n\n");
}

void fs_init(void) {
    memset(disk1, 0, sizeof(disk1));
    memset(disk2, 0, sizeof(disk2));
    memset(block_checksums, 0, sizeof(block_checksums));
    memset(block_used, 0, sizeof(block_used));
    memset(inode_table, 0, sizeof(inode_table));

    disk1_online = 1;
    disk2_online = 1;
    self_heal_count = 0;
}

int allocate_block(void) {
    for (int i = 0; i < TOTAL_BLOCKS; i++) {
        if (block_used[i] == 0) {
            block_used[i] = 1;
            return i;
        }
    }
    return -1;
}

void free_block(int block_num) {
    if (block_num >= 0 && block_num < TOTAL_BLOCKS) {
        block_used[block_num] = 0;
        uint8_t zero_buffer[BLOCK_SIZE] = {0};
        raid1_write_block(block_num, zero_buffer);
    }
}

int allocate_inode(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (inode_table[i].is_used == 0) {
            memset(&inode_table[i], 0, sizeof(Inode));
            inode_table[i].is_used = 1;
            inode_table[i].ref_count = 1;
            return i;
        }
    }
    return -1;
}

void free_inode(int inode_id) {
    if (inode_id >= 0 && inode_id < MAX_FILES) {
        memset(&inode_table[inode_id], 0, sizeof(Inode));
    }
}

Inode* get_inode(int inode_id) {
    if (inode_id >= 0 && inode_id < MAX_FILES && inode_table[inode_id].is_used) {
        return &inode_table[inode_id];
    }
    return NULL;
}

int find_inode_by_name(const char *name) {
    if (name == NULL) return -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (inode_table[i].is_used && strcmp(inode_table[i].filename, name) == 0) {
            return i;
        }
    }
    return -1;
}