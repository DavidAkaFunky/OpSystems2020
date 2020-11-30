#ifndef FS_H
#define FS_H
#include "state.h"

void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create_aux(char *name, type nodeType);
int create(char *name, type nodeType, int *activeLocks, int *numActiveLocks);
int delete_aux(char *name);
int delete(char * name, int * activeLocks, int * numActiveLocks);
int lookup_aux(char * name);
int lookup(char *name, int * activeLocks, int * numActiveLocks, bool write);
int move_aux(char* oldPath, char* newPath);
int move(char* oldPath, char* newPath, int* activeLocks, int* numActiveLocks);
int print_tecnicofs_tree(FILE *fp);

#endif /* FS_H */
