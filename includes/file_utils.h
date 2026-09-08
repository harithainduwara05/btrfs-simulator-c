#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include "fs_core.h"

int fs_copy_file(const char *src_filename, const char *dest_filename);
int fs_delete_file(const char *filename);
int fs_read_file(const char *filename, void *buffer, int max_len);

#endif