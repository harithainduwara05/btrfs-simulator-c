#include "../includes/fs_core.h"
#include "../includes/file_ops.h"
#include "../includes/file_utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void print_help(int dev_mode) {
    printf("\n======================= COMMAND LIST =======================\n");
    printf(" fcreate <filename>          : Create a new file with content\n");
    printf(" fcopy <source> <dest>       : Copy file (Btrfs Copy-on-Write)\n");
    printf(" fread <filename>            : Read and display file content\n");
    printf(" fdel <filename>             : Delete an existing file\n");
    printf(" show-all                    : List all files, sizes & blocks\n");
    printf(" dev-m                       : Enter Developer Mode (Password: 1234)\n");
    printf(" help                        : Show available commands\n");
    printf(" exit                        : Exit the simulator\n");

    if (dev_mode) {
        printf("\n------------- DEVELOPER & RAID-1 TEST COMMANDS -------------\n");
        printf(" show-alldev                 : Show detailed file mapping & block numbers\n");
        printf(" fdamage <disk_id> <block>   : Corrupt a block to test Self-Healing\n");
        printf("                               (Example: fdamage 1 0)\n");
        printf(" fdisk <disk_id> <1|0>       : Set disk status (1 = Online, 0 = Offline)\n");
        printf("                               (Example: fdisk 1 0 to fail Disk 1)\n");
        printf(" fstatus                     : Show RAID-1 health & self-heal stats\n");
        printf(" dev-exit                    : Return to normal user mode\n");
    }
    printf("============================================================\n\n");
}

static void fs_list_files_dev(void) {
    int found = 0;
    printf("\n------------------------------ DEV FILE & BLOCK MAPPING ------------------------------\n");
    printf(" %-3s %-25s %8s %7s %8s %-25s\n", "ID", "Filename", "Size(B)", "Blocks", "RefCnt", "Allocated Block IDs");
    printf("--------------------------------------------------------------------------------------\n");

    for (int i = 0; i < MAX_FILES; i++) {
        Inode *inode = get_inode(i);
        if (inode != NULL) {
            printf(" %-3d %-25s %8d %7d %8d [",
                   i, inode->filename, inode->size, inode->block_count, inode->ref_count);
            for (int b = 0; b < inode->block_count; b++) {
                printf("%d%s", inode->block_pointers[b], (b == inode->block_count - 1) ? "" : ", ");
            }
            printf("]\n");
            found++;
        }
    }

    if (found == 0) {
        printf("                       (no files found on the filesystem)\n");
    }

    printf("--------------------------------------------------------------------------------------\n");
    printf(" Total files: %d / %d\n\n", found, MAX_FILES);
}

int main(void) {
    fs_init();

    int dev_mode = 0;
    char line[1024];

    printf("\n============================================================\n");
    printf("           BTRFS SIMULATOR (RAID-1 & CoW ENABLED)           \n");
    printf("============================================================\n");
    printf("Type 'help' to see all commands. Type 'exit' to quit.\n\n");

    while (1) {
        if (dev_mode) {
            printf("btrfs(dev)> ");
        } else {
            printf("btrfs> ");
        }
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }

        line[strcspn(line, "\r\n")] = '\0';

        if (strlen(line) == 0) {
            continue;
        }

        char command[64] = {0};
        char arg1[MAX_FILENAME] = {0};
        char arg2[MAX_FILENAME] = {0};

        int parsed = sscanf(line, "%63s %127s %127s", command, arg1, arg2);

        if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
            printf("Exiting BTRFS Simulator. Goodbye!\n");
            break;
        } else if (strcmp(command, "help") == 0) {
            print_help(dev_mode);
        } else if (strcmp(command, "show-all") == 0) {
            fs_list_files();
        } else if (strcmp(command, "show-alldev") == 0) {
            if (!dev_mode) {
                printf("[RESTRICTED] 'show-alldev' requires Developer Mode. Type 'dev-m' first.\n");
                continue;
            }
            fs_list_files_dev();
        } else if (strcmp(command, "fcreate") == 0) {
            if (parsed < 2) {
                printf("[USAGE] fcreate <filename>\n");
                continue;
            }

            char content[2048] = {0};
            printf("content: ");
            fflush(stdout);

            if (fgets(content, sizeof(content), stdin) != NULL) {
                content[strcspn(content, "\r\n")] = '\0';
                fs_create_file(arg1, content, (int)strlen(content));
            }
        } else if (strcmp(command, "fcopy") == 0) {
            if (parsed < 3) {
                printf("[USAGE] fcopy <source_file> <dest_file>\n");
                continue;
            }
            fs_copy_file(arg1, arg2);
        } else if (strcmp(command, "fread") == 0) {
            if (parsed < 2) {
                printf("[USAGE] fread <filename>\n");
                continue;
            }

            char read_buffer[4096] = {0};
            int bytes = fs_read_file(arg1, read_buffer, sizeof(read_buffer) - 1);
            if (bytes >= 0) {
                printf("\n----- [%s (%d bytes)] -----\n", arg1, bytes);
                printf("%s\n", read_buffer);
                printf("-----------------------------------------\n\n");
            }
        } else if (strcmp(command, "fdel") == 0) {
            if (parsed < 2) {
                printf("[USAGE] fdel <filename>\n");
                continue;
            }
            fs_delete_file(arg1);
        } else if (strcmp(command, "dev-m") == 0) {
            if (dev_mode) {
                printf("You are already in Developer Mode.\n");
                continue;
            }

            char password[64] = {0};
            printf("Enter Developer Password: ");
            fflush(stdout);

            if (fgets(password, sizeof(password), stdin) != NULL) {
                password[strcspn(password, "\r\n")] = '\0';
                if (strcmp(password, "1234") == 0) {
                    dev_mode = 1;
                    printf("\n[ACCESS GRANTED] Switched to Developer Mode!\n");
                    printf("Unlocked commands: 'fdamage', 'fdisk', 'fstatus', 'dev-exit'\n\n");
                } else {
                    printf("[ACCESS DENIED] Incorrect password!\n");
                }
            }
        } else if (strcmp(command, "dev-exit") == 0) {
            if (!dev_mode) {
                printf("You are already in normal user mode.\n");
            } else {
                dev_mode = 0;
                printf("[DEV MODE DEACTIVATED] Returned to standard user mode.\n");
            }
        } else if (strcmp(command, "fdamage") == 0) {
            if (!dev_mode) {
                printf("[RESTRICTED] 'fdamage' requires Developer Mode. Type 'dev-m' first.\n");
                continue;
            }

            if (parsed < 2) {
                printf("[USAGE] fdamage <block_num> OR fdamage <disk_id> <block_num>\n");
                continue;
            }

            int disk_id = 1;
            int block_num = -1;

            if (parsed >= 3) {
                disk_id = atoi(arg1);
                block_num = atoi(arg2);
            } else {
                block_num = atoi(arg1);
            }

            if (disk_id < 1 || disk_id > 2) {
                printf("[ERROR] Invalid disk_id. Must be 1 or 2.\n");
                continue;
            }
            if (block_num < 0 || block_num >= TOTAL_BLOCKS) {
                printf("[ERROR] Invalid block_num. Must be between 0 and %d.\n", TOTAL_BLOCKS - 1);
                continue;
            }

            raid1_corrupt_block(disk_id, block_num);
            printf("[NOTE] Try using 'fread' on a file that uses block %d to see Btrfs Self-Healing in action!\n", block_num);
        } else if (strcmp(command, "fdisk") == 0) {
            if (!dev_mode) {
                printf("[RESTRICTED] 'fdisk' requires Developer Mode. Type 'dev-m' first.\n");
                continue;
            }

            if (parsed < 3) {
                printf("[USAGE] fdisk <disk_id: 1|2> <status: 1(Online)|0(Offline)>\n");
                continue;
            }

            int disk_id = atoi(arg1);
            int status = atoi(arg2);

            if (disk_id < 1 || disk_id > 2) {
                printf("[ERROR] Invalid disk_id. Must be 1 or 2.\n");
                continue;
            }

            raid1_set_disk_status(disk_id, status ? 1 : 0);
        } else if (strcmp(command, "fstatus") == 0) {
            if (!dev_mode) {
                printf("[RESTRICTED] 'fstatus' requires Developer Mode. Type 'dev-m' first.\n");
                continue;
            }
            raid1_print_status();
        } else {
            printf("[UNKNOWN COMMAND] '%s'. Type 'help' to see available commands.\n", command);
        }
    }

    return 0;
}