#ifndef INODES_H
#define INODES_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include "../tecnicofs-api-constants.h"

/* FS root inode number */
#define FS_ROOT 0

#define FREE_INODE -1
#define INODE_TABLE_SIZE 50
#define MAX_DIR_ENTRIES 20

#define SUCCESS 0
#define FAIL -1

#define DELAY 5000000

/*
 * Contains the name of the entry and respective i-number
 */
typedef struct dirEntry {
	char name[MAX_FILE_NAME];
	int inumber;
} DirEntry;

/*
 * Data is either text (file) or entries (DirEntry)
 */
union Data {
	char *fileContents; /* for files */
	DirEntry *dirEntries; /* for directories */
};

/*
 * I-node definition
 */
typedef struct inode_t {    
	type nodeType;
	union Data data;
	pthread_rwlock_t rwl; /* Trinco fino */
    /* more i-node attributes will be added in future exercises */
} inode_t;

inode_t inode_table[INODE_TABLE_SIZE];

void insert_delay(int cycles);
void inode_table_init();
void inode_table_destroy();
int inode_create(type nType);
int inode_delete(int inumber);
int inode_get(int inumber, type *nType, union Data *data);
int lookup_sub_node(char *name, DirEntry *entries, bool write);
int dir_reset_entry(int inumber, int sub_inumber);
int dir_add_entry(int inumber, int sub_inumber, char *sub_name);
int tryLock(int inumber, int lockType);
int lock(int inumber, int lockType);
int lockRead(int inumber);
int lockWrite(int inumber);
int tryLockRead(int inumber);
int tryLockWrite(int inumber);
void unlock(int inumber);
void unlockAll(int inumbers[], int size);
void inode_print_tree(FILE *fp, int inumber, char *name);

#endif /* INODES_H */
