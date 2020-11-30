#include "operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Given a path, fills pointers with strings for the parent path and child
 * file name
 * Input:
 *  - path: the path to split. ATTENTION: the function may alter this parameter
 *  - parent: reference to a char*, to store parent path
 *  - child: reference to a char*, to store child file name
 */
void split_parent_child_from_path(char * path, char ** parent, char ** child) {

	int n_slashes = 0, last_slash_location = 0;
	int len = strlen(path);

	// deal with trailing slash ( a/x vs a/x/ )
	if (path[len-1] == '/') {
		path[len-1] = '\0';
	}

	for (int i=0; i < len; ++i) {
		if (path[i] == '/' && path[i+1] != '\0') {
			last_slash_location = i;
			n_slashes++;
		}
	}

	if (n_slashes == 0) { // root directory
		*parent = "";
		*child = path;
		return;
	}

	path[last_slash_location] = '\0';
	*parent = path;
	*child = path + last_slash_location + 1;

}


/*
 * Initializes tecnicofs and creates root node.
 */
void init_fs() {

	inode_table_init();
	
	/* create root inode */
	int root = inode_create(T_DIRECTORY);
	unlock(root);

	if (root != FS_ROOT) {
		printf("failed to create node for tecnicofs root\n");
		exit(EXIT_FAILURE);
	}
}

/*
 * Destroy tecnicofs and inode table.
 */
void destroy_fs() {
	inode_table_destroy();
}


/*
 * Checks if content of directory is not empty.
 * Input:
 *  - entries: entries of directory
 * Returns: SUCCESS or FAIL
 */
int is_dir_empty(DirEntry *dirEntries) {
	if (dirEntries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		if (dirEntries[i].inumber != FREE_INODE) {
			return FAIL;
		}
	}
	return SUCCESS;
}

/**
 * Calls create function with local variables.
 * Input:
 *  - name: path of node
 *  - nodeType: type of node
 * Returns: SUCCESS or FAIL
 */
int create_aux(char *name, type nodeType) {
	int activeLocks[INODE_TABLE_SIZE], numActiveLocks = 0;
	int retVal = create(name, nodeType, activeLocks, &numActiveLocks);
	unlockAll(activeLocks, numActiveLocks);
	return retVal;
}

/*
 * Creates a new node given a path.
 * Input:
 *  - name: path of node
 *  - nodeType: type of node
 *  - activeLocks: array containing active locks
 *  - numActiveLocks: activeLocks's length
 * Returns: SUCCESS or FAIL
 */
int create(char *name, type nodeType, int *activeLocks, int *numActiveLocks) {
	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType;
	union Data pdata;

	strcpy(name_copy, name);
	/* produces path to child : i.e. "c s1/s2/s3 d" -> parent_name = "s1/s2" ; child_name = "s3" */
	/* idem to file */
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookup(parent_name, activeLocks, numActiveLocks, true);

	if (parent_inumber == FAIL) {
		printf("failed to create %s, invalid parent dir %s\n",
		        name, parent_name);
		return FAIL;
	}

	/* get all data related to parent */
	inode_get(parent_inumber, &pType, &pdata);

	/* parent needs to be a directory */
	if(pType != T_DIRECTORY) {
		printf("failed to create %s, parent %s is not a dir\n",
		        name, parent_name);
		return FAIL;
	}

	/* if file already exists, can't create it */
	if (lookup_sub_node(child_name, pdata.dirEntries) != FAIL) {
		printf("failed to create %s, already exists in dir %s\n",
		       child_name, parent_name);
		return FAIL;
	}

	/* create node and add entry to folder that contains new node */
	child_inumber = inode_create(nodeType);

	/* inode_create returns FAIL if anything went wrong */
	if (child_inumber == FAIL) {
		printf("failed to create %s in %s, couldn't allocate inode\n",
		        child_name, parent_name);
		return FAIL;
	}

	activeLocks[(*numActiveLocks)++] = child_inumber;

	/* add entry to parent directory */
	if (dir_add_entry(parent_inumber, child_inumber, child_name) == FAIL) {
		printf("could not add entry %s in dir %s\n",
		       child_name, parent_name);
		return FAIL;
	}

	return SUCCESS;
}

/**
 * Calls delete function with local variables.
 * Input:
 *  - name: path of node
 * Returns: SUCCESS or FAIL
 */
int delete_aux(char *name) {
	int activeLocks[INODE_TABLE_SIZE], numActiveLocks = 0;
	int retVal = delete(name, activeLocks, &numActiveLocks);
	unlockAll(activeLocks, numActiveLocks);
	return retVal;
}

/*
 * Deletes a node given a path.
 * Input:
 *  - name: path of node
 *  - activeLocks: array containing active locks
 *  - numActiveLocks: activeLocks's length
 * Returns: SUCCESS or FAIL
 */
int delete(char * name, int * activeLocks, int * numActiveLocks) {
	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];

	/* use for copy */
	type pType, cType;
	union Data pdata, cdata;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookup(parent_name, activeLocks, numActiveLocks, true);

	if (parent_inumber == FAIL) {
		printf("failed to delete %s, invalid parent dir %s\n",
		        child_name, parent_name);
		return FAIL;
	}
	
	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		printf("failed to delete %s, parent %s is not a dir\n",
		        child_name, parent_name);
		return FAIL;
	}

	child_inumber = lookup_sub_node(child_name, pdata.dirEntries);

	if (child_inumber == FAIL) {
		printf("could not delete %s, does not exist in dir %s\n",
		       name, parent_name);
		return FAIL;
	}

	lock(child_inumber, WRITE);
	activeLocks[(*numActiveLocks)++] = child_inumber;

	inode_get(child_inumber, &cType, &cdata);

	if (cType == T_DIRECTORY && is_dir_empty(cdata.dirEntries) == FAIL) {
		printf("could not delete %s: is a directory and not empty\n",
		       name);
		return FAIL;
	}

	/* remove entry from folder that contained deleted node */
	if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
		printf("failed to delete %s from dir %s\n",
		       child_name, parent_name);
		return FAIL;
	}

	if (inode_delete(child_inumber) == FAIL) {
		printf("could not delete inode number %d from dir %s\n",
		       child_inumber, parent_name);
		return FAIL;
	}

	return SUCCESS;
}

/**
 * Checks if inumber is currently locked.
 * Input:
 *  - inumber: number being checked
 *  - activeLocks: array containing active locks
 *  - numActiveLocks: activeLocks's length
 */
int isLocked(int inumber, int * activeLocks, int numActiveLocks) {
	for (int i = 0; i < numActiveLocks; ++i) {
		if (activeLocks[i] == inumber) {
			return 1;
		}
	}
	return 0;
}

/*
 * Calls lookup function with local variables.
 * Input:
 *  - name: path of node
 * Returns: 
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int lookup_aux(char * name) {
	int activeLocks[INODE_TABLE_SIZE], numActiveLocks = 0;
	int search = lookup(name, activeLocks, &numActiveLocks, false);
	unlockAll(activeLocks, numActiveLocks);
	return search;
}

/*
 * Lookup for a given path.
 * Input:
 *  - name: path of node
 *  - activeLocks: array containing active locks
 *  - numActiveLocks: activeLocks's length
 *  - write: if true, the last node on lock will be locked on WRITE
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int lookup(char *name, int * activeLocks, int * numActiveLocks, bool write) {
	char full_path[MAX_FILE_NAME];
	char delim[] = "/";
	char * saveptr;
	
	strcpy(full_path, name);

	/* start at root node */
	int current_inumber = FS_ROOT;
	/* use for copy */
	type nType;
	union Data data;
	char *path = strtok_r(full_path, delim, &saveptr);

	/* Avoid double locks and validate current_inumber's ~lock status */
	if (!isLocked(current_inumber, activeLocks, *numActiveLocks)) {
		if (!path && write) {
			lock(current_inumber, WRITE);
		} else {
			lock(current_inumber, READ);
		}
		activeLocks[(*numActiveLocks)++] = current_inumber;
	}

	/* get root inode data */
	inode_get(current_inumber, &nType, &data);

	/* search for all sub nodes */
	while (path && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		path = strtok_r(NULL, delim, &saveptr);
		if (!isLocked(current_inumber, activeLocks, *numActiveLocks)) {
			if (!path && write) {
				lock(current_inumber, WRITE);
			} else {
				lock(current_inumber, READ);
			}
			activeLocks[(*numActiveLocks)++] = current_inumber;
		}
		inode_get(current_inumber, &nType, &data);
	}

	return current_inumber;
}

/**
 * Calls move function with local variables
 * Input:
 *  - oldPath: path of node to be moved. 
 *  - newPath: path where i-node will be moved to
 * Returns: SUCCESS or FAIL
 */
int move_aux(char * oldPath, char * newPath) {
	int activeLocks[INODE_TABLE_SIZE], numActiveLocks = 0;
	int search = move(oldPath, newPath, activeLocks, &numActiveLocks);
	unlockAll(activeLocks, numActiveLocks);
	return search;
}

/*
 * Moves a node from a given path to another given path.
 * Input:
 *  - oldPath: path of node to be moved. 
 *  - newPath: path where i-node will be moved to
 *  - activeLocks: array containing active locks
 *  - numActiveLocks: activeLocks's length
 * Returns: SUCCESS or FAIL
 */
int move(char * oldPath, char * newPath, int * activeLocks, int * numActiveLocks) {
	char *old_parent_name, *old_child_name, oldPath_copy[MAX_FILE_NAME];
	char *new_parent_name, *new_child_name, newPath_copy[MAX_FILE_NAME];
	int old_parent_inumber, new_parent_inumber, moving_inumber;
	union Data old_pdata, new_pdata;
	type old_pType, new_pType;

	strcpy(oldPath_copy, oldPath);
	split_parent_child_from_path(oldPath_copy, &old_parent_name, &old_child_name);

	strcpy(newPath_copy, newPath);
	split_parent_child_from_path(newPath_copy, &new_parent_name, &new_child_name);

	/* Alphabetical way to order locks */
	if (strcmp(old_parent_name, new_parent_name) < 0) {
		old_parent_inumber = lookup(old_parent_name, activeLocks, numActiveLocks, true);
		new_parent_inumber = lookup(new_parent_name, activeLocks, numActiveLocks, true);		 
	} else {
		new_parent_inumber = lookup(new_parent_name, activeLocks, numActiveLocks, true);
		old_parent_inumber = lookup(old_parent_name, activeLocks, numActiveLocks, true);
	}

	/* Invalid paths */
	if (old_parent_inumber == FAIL || new_parent_inumber == FAIL) {
		printf("failed to move, invalid input paths\n");
		return FAIL;
	}

	inode_get(old_parent_inumber, &old_pType, &old_pdata);
	inode_get(new_parent_inumber, &new_pType, &new_pdata);

	/* newPath -> parent is not a directory */
	if (new_pType != T_DIRECTORY) {
		printf("failed to move, new path's parent i-node is not a directory\n");
		return FAIL;
	}

	/* newPath -> parent directory contains an entry with same name as the moving i-node */
	if (lookup_sub_node(new_child_name, new_pdata.dirEntries) != FAIL) {
		printf("failed to move, new parent directory already contains an entry named %s\n", old_child_name);
		return FAIL;
	}

	/* oldPath -> check if moving i-node exists */
	if ((moving_inumber = lookup_sub_node(old_child_name, old_pdata.dirEntries)) == FAIL) {
		printf("failed to move, old parent directory doesn't contain an entry named %s\n", old_child_name);
		return FAIL;
	}

	/* oldPath & newPath -> avoid reflexing moves */
	if (moving_inumber == new_parent_inumber) {
		printf("failed to move, can't move directory to itself\n");
		return FAIL;
	}

	/* reset oldPath entry and add new entry to newPath */
	if (dir_reset_entry(old_parent_inumber, moving_inumber) == FAIL) {
		printf("failed to move, couldn't reset %s from dir %s\n", old_child_name, old_parent_name);
		return FAIL;
	}

	if (dir_add_entry(new_parent_inumber, moving_inumber, new_child_name) == FAIL) {
		printf("failed to move, couldn't add %s to dir %s\n", new_child_name, new_parent_name);
		return FAIL;
	}

	return SUCCESS;

}

/*
 * Prints tecnicofs tree.
 * Input:
 *  - fp: pointer to output file
 */
int print_tecnicofs_tree(FILE *fp) {
	lock(FS_ROOT, WRITE);
	int res = inode_print_tree(fp, FS_ROOT, "");
	unlock(FS_ROOT);
	return res;
}

