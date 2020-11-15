#ifndef FS_H
#define FS_H
#include "state.h"

void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType);
int delete(char *name);
int lookup(char *name, int * activeLocks, int * numActiveLocks, bool write);
void compareOldNewPaths(const char * oldPath, const char * newPath, int * flag);
void resetActiveLocks(int * activeLocks, int * numActiveLocks);
int checkOldPath(char * oldPath, int * activeLocks, int * numActiveLocks, int * refInumber);
int checkNewPath(char * newPath, int * activeLocks, int * numActiveLocks, int inumber);
int lookupMove(char *name, int * activeLocks, int * numActiveLocks);
int move(char* oldPath, char* newPath);
void print_tecnicofs_tree(FILE *fp);
void destroy_locks();

#endif /* FS_H */
