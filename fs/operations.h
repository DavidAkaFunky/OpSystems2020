#ifndef FS_H
#define FS_H
#include "state.h"
#include <stdbool.h>

void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType);
int delete(char *name);
int lookup(char *name, int * activeLocks, int * j, bool write);
void print_tecnicofs_tree(FILE *fp);
void destroy_locks();

#endif /* FS_H */
