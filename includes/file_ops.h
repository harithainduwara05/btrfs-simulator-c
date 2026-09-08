//Create, list function declarations
// Create, list function declarations
#ifndef FILE_OPS_H
#define FILE_OPS_H
 
#include "fs_core.h"
 
// Creates a new file with the given name and writes 'size' bytes from
// 'data' into it, spreading the data across blocks via the RAID-1 layer.
// Returns the inode id on success, or -1 on failure (name too long,
// duplicate name, not enough free blocks/inodes, file too large).
int fs_create_file(const char *filename, const void *data, int size);
 
// Prints a directory-style listing of every file currently on the
// simulated filesystem: name, size in bytes, and number of blocks used.
void fs_list_files(void);
 
#endif
 
