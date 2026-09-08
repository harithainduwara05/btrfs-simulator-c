/*
 * Standalone test for fs_create_file / fs_list_files.
 * NOT part of the real build - do not add this to main.c or the Makefile.
 * Compile it on its own:
 *   gcc -Wall -Wextra -Iincludes src/fs_core.c src/file_ops.c test/test_create_interactive.c -o test_create.exe
 * Run it:
 *   ./test_create.exe
 */

#include "../includes/fs_core.h"
#include "../includes/file_ops.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    fs_init();

    char filename[MAX_FILENAME];
    char content[512];

    printf("===== File Create Test =====\n");

    while (1) {
        printf("\nEnter filename (or 'done' to finish): ");
        if (fgets(filename, sizeof(filename), stdin) == NULL) break;
        filename[strcspn(filename, "\n")] = '\0'; /* strip newline */

        if (strcmp(filename, "done") == 0) break;
        if (strlen(filename) == 0) continue;

        printf("Enter file content: ");
        if (fgets(content, sizeof(content), stdin) == NULL) break;
        content[strcspn(content, "\n")] = '\0';

        fs_create_file(filename, content, (int)strlen(content));
    }

    fs_list_files();
    return 0;
}