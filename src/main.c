#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Header File Handling */
#include "../includes/fs_core.h"
#include "../includes/file_ops.h"
#include "../includes/file_utils.h"

/* Buffer clearing utility for safe CLI input */
static void clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/* CLI Menu Display */
static void print_menu(void) {
    printf("\n======================================================\n");
    printf("         BTRFS SIMULATION SYSTEM (RAID-1)             \n");
    printf("======================================================\n");
    printf(" 1. Create New File\n");
    printf(" 2. List Files\n");
    printf(" 3. Copy File\n");
    printf(" 4. Delete File\n");
    printf(" 5. Check RAID-1 Status\n");
    printf(" 6. Simulate Block Corruption\n");
    printf(" 7. Toggle Disk Online/Offline Status\n");
    printf(" 0. Exit System\n");
    printf("======================================================\n");
    printf("Enter choice: ");
}

int main(void) {
    /* Initialize File System Core */
    fs_init();
    
    int choice;
    char filename[MAX_FILENAME];
    char dest_filename[MAX_FILENAME];
    char content[1024];

    while (1) {
        print_menu();

        if (scanf("%d", &choice) != 1) {
            printf("[CLI ERROR] Invalid input! Please enter a number.\n");
            clear_input_buffer();
            continue;
        }
        clear_input_buffer();

        switch (choice) {
            case 1:
                /* Create File */
                printf("\nEnter Filename to create: ");
                if (fgets(filename, sizeof(filename), stdin) != NULL) {
                    filename[strcspn(filename, "\r\n")] = '\0';
                    
                    if (strlen(filename) == 0) {
                        printf("[CLI ERROR] Filename cannot be empty!\n");
                        break;
                    }

                    printf("Enter File Content: ");
                    if (fgets(content, sizeof(content), stdin) != NULL) {
                        content[strcspn(content, "\r\n")] = '\0';
                        fs_create_file(filename, content, (int)strlen(content));
                    }
                }
                break;

            case 2:
                /* List Files */
                fs_list_files();
                break;

            case 3:
                /* Copy File */
                printf("\nEnter Source Filename: ");
                if (fgets(filename, sizeof(filename), stdin) != NULL) {
                    filename[strcspn(filename, "\r\n")] = '\0';

                    printf("Enter Destination Filename: ");
                    if (fgets(dest_filename, sizeof(dest_filename), stdin) != NULL) {
                        dest_filename[strcspn(dest_filename, "\r\n")] = '\0';
                        fs_copy_file(filename, dest_filename);
                    }
                }
                break;

            case 4:
                /* Delete File */
                printf("\nEnter Filename to delete: ");
                if (fgets(filename, sizeof(filename), stdin) != NULL) {
                    filename[strcspn(filename, "\r\n")] = '\0';
                    fs_delete_file(filename);
                }
                break;

            case 5:
                /* RAID-1 Status */
                raid1_print_status();
                break;

            case 6: {
                /* Simulate Block Corruption for Self-Heal Demo */
                int disk_id, block_num;
                printf("\nEnter Disk ID to corrupt (1 or 2): ");
                if (scanf("%d", &disk_id) == 1) {
                    printf("Enter Block Number (0 - %d): ", TOTAL_BLOCKS - 1);
                    if (scanf("%d", &block_num) == 1) {
                        raid1_corrupt_block(disk_id, block_num);
                    }
                }
                clear_input_buffer();
                break;
            }

            case 7: {
                /* Toggle Disk Status */
                int disk_id, status;
                printf("\nEnter Disk ID (1 or 2): ");
                if (scanf("%d", &disk_id) == 1) {
                    printf("Enter Status (1 for ONLINE, 0 for OFFLINE): ");
                    if (scanf("%d", &status) == 1) {
                        raid1_set_disk_status(disk_id, status);
                    }
                }
                clear_input_buffer();
                break;
            }

            case 0:
                printf("\nExiting Btrfs Simulator. Goodbye!\n");
                return 0;

            default:
                printf("\n[CLI ERROR] Invalid choice! Option not recognized.\n");
                break;
        }
    }

    return 0;
}