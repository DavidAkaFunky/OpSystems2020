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


/*
 * Creates a new node given a path.
 * Input:
 *  - name: path of node
 *  - nodeType: type of node
 * Returns: SUCCESS or FAIL
 */
int create(char *name, type nodeType){

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType;
	union Data pdata;

	int activeLocks[INODE_TABLE_SIZE], j = 0;

	strcpy(name_copy, name);
	/* produces path to child : i.e. "c s1/s2/s3 d" -> parent_name = "s1/s2" ; child_name = "s3" */
	/* idem to file */
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookup(parent_name, activeLocks, &j, true);

	if (parent_inumber == FAIL) {
		printf("failed to create %s, invalid parent dir %s\n",
		        name, parent_name);
		unlockAll(activeLocks, j);
		return FAIL;
	}

	/* get all data related to parent */
	inode_get(parent_inumber, &pType, &pdata);

	/* parent needs to be a directory */
	if(pType != T_DIRECTORY) {
		printf("failed to create %s, parent %s is not a dir\n",
		        name, parent_name);
		unlockAll(activeLocks, j);
		return FAIL;
	}

	/* if file already exists, can't create it */
	if (lookup_sub_node(child_name, pdata.dirEntries, false) != FAIL) {
		printf("failed to create %s, already exists in dir %s\n",
		       child_name, parent_name);
		unlockAll(activeLocks, j);
		return FAIL;
	}

	/* create node and add entry to folder that contains new node */
	child_inumber = inode_create(nodeType);
	
	/* inode_create returns FAIL if anything went wrong */
	if (child_inumber == FAIL) {
		printf("failed to create %s in %s, couldn't allocate inode\n",
		        child_name, parent_name);
		unlockAll(activeLocks, j);
		return FAIL;
	}

	activeLocks[j++] = child_inumber;

	/* add entry to parent directory */
	if (dir_add_entry(parent_inumber, child_inumber, child_name) == FAIL) {
		printf("could not add entry %s in dir %s\n",
		       child_name, parent_name);
		unlockAll(activeLocks, j);
		return FAIL;
	}

	unlockAll(activeLocks, j);
	return SUCCESS;
}


/*
 * Deletes a node given a path.
 * Input:
 *  - name: path of node
 * Returns: SUCCESS or FAIL
 */
int delete(char *name){

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType, cType;
	union Data pdata, cdata;

	int activeLocks[INODE_TABLE_SIZE], j = 0;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookup(parent_name, activeLocks, &j, true);

	if (parent_inumber == FAIL) {
		printf("failed to delete %s, invalid parent dir %s\n",
		        child_name, parent_name);
		unlockAll(activeLocks, j);
		return FAIL;
	}
	
	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		printf("failed to delete %s, parent %s is not a dir\n",
		        child_name, parent_name);
		unlockAll(activeLocks, j);
		return FAIL;
	}

	child_inumber = lookup_sub_node(child_name, pdata.dirEntries, true);

	if (child_inumber == FAIL) {
		printf("could not delete %s, does not exist in dir %s\n",
		       name, parent_name);
		unlockAll(activeLocks, j);
		return FAIL;
	}

	activeLocks[j++] = child_inumber;

	inode_get(child_inumber, &cType, &cdata);

	if (cType == T_DIRECTORY && is_dir_empty(cdata.dirEntries) == FAIL) {
		printf("could not delete %s: is a directory and not empty\n",
		       name);
		unlockAll(activeLocks, j);
		return FAIL;
	}

	/* remove entry from folder that contained deleted node */
	if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
		printf("failed to delete %s from dir %s\n",
		       child_name, parent_name);
		unlockAll(activeLocks, j);
		return FAIL;
	}

	/* inode_delete will unlock its rwlock */
	for (int i = 0; i < j; ++i) {
		if (activeLocks[i] == child_inumber) {
			activeLocks[i] = -1;
		}
	}

	if (inode_delete(child_inumber) == FAIL) {
		printf("could not delete inode number %d from dir %s\n",
		       child_inumber, parent_name);
		unlockAll(activeLocks, j);
		return FAIL;
	}
	unlockAll(activeLocks, --j);
	return SUCCESS;
}


/*
 * Lookup for a given path.
 * Input:
 *  - name: path of node
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int lookup(char *name, int * activeLocks, int * j, bool write) {
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
	/* inode creation at root */
	if (!path && write) {
		lock(current_inumber, WRITE);
	} else {
		lock(current_inumber, READ);
	}
	/* get root inode data */
	inode_get(current_inumber, &nType, &data);
	activeLocks[(*j)++] = current_inumber;

	/* search for all sub nodes */
	while (path && (current_inumber = lookup_sub_node(path, data.dirEntries, false)) != FAIL) {
		inode_get(current_inumber, &nType, &data);
		path = strtok_r(NULL, delim, &saveptr);
		if (!path && write) {
			if (!lock(current_inumber, WRITE)) {
				activeLocks[(*j)++] = current_inumber;
			}
			return current_inumber;
		} else {
			if (!lock(current_inumber, READ)) {
				activeLocks[(*j)++] = current_inumber;
			}
		}
	}

	return current_inumber;
}

void compareOldNewPaths(const char * oldPath, const char * newPath, int * flag) {
	int oldCounter = 0, newCounter = 0;
	for (int i = 0; i < strlen(oldPath); ++i) {
		if (oldPath[i] == '/') {
			oldCounter++;
		}
	}
	for (int i = 0; i < strlen(newPath); ++i) {
		if (newPath[i] == '/') {
			newCounter++;
		}
	}
	*flag = oldCounter < newCounter ? 1 : 0;
}

void resetActiveLocks(int * activeLocks, int * numActiveLocks) {
	unlockAll(activeLocks, *numActiveLocks);
	memset(activeLocks, 0, sizeof(*activeLocks));
 	*numActiveLocks = 0;
}

int checkOldPath(char * oldPath, int * activeLocks, int * numActiveLocks, int * refInumber) {
	int inumber;
	if ((inumber = lookup(oldPath, activeLocks, numActiveLocks, false)) == FAIL) {
		unlockAll(activeLocks, *numActiveLocks);
		return FAIL;
	}
	*refInumber = inumber;
	resetActiveLocks(activeLocks, numActiveLocks);
	return SUCCESS;
}

int checkNewPath(char * newPath, int * activeLocks, int * numActiveLocks) {
	char *new_parent_name, *new_child_name, newPath_copy[MAX_FILE_NAME];
	strcpy(newPath_copy, newPath);
	split_parent_child_from_path(newPath_copy, &new_parent_name, &new_child_name);
	int new_parent_inumber = lookup(new_parent_name, activeLocks, numActiveLocks, false);
	resetActiveLocks(activeLocks, numActiveLocks);
	union Data new_pdata;
	type new_pType;
	inode_get(new_parent_inumber, &new_pType, &new_pdata);
	if (new_parent_inumber == FAIL) {
		printf("failed to move, there's no file or dir with the new parent name in %s\n", newPath);
		unlockAll(activeLocks, *numActiveLocks);
		return FAIL;
	}
	else if (new_pType != T_DIRECTORY) {
		printf("failed to move, parent name in %s not a directory\n", newPath);
		unlockAll(activeLocks, *numActiveLocks);
		return FAIL;
	} else if (lookup(newPath, activeLocks, numActiveLocks, false) != FAIL) {
		printf("failed to move, there's a file or dir with the new path name\n");
		unlockAll(activeLocks, *numActiveLocks);
		return FAIL;
	}
	resetActiveLocks(activeLocks, numActiveLocks);
	return SUCCESS;
}

int move(char* oldPath, char* newPath) {
	int activeLocks[INODE_TABLE_SIZE], numActiveLocks = 0;
	int inumber;
	int flag;
	if (checkOldPath(oldPath, activeLocks, &numActiveLocks, &inumber) || 
	checkNewPath(newPath, activeLocks, &numActiveLocks)) { 
		return FAIL;
	}

	compareOldNewPaths(oldPath, newPath, &flag);

	/* First lock the oldPath parent directory (shortest Path) and then newPath parent */
	char *old_parent_name, *old_child_name, oldPath_copy[MAX_FILE_NAME];
	strcpy(oldPath_copy, oldPath);
	split_parent_child_from_path(oldPath_copy, &old_parent_name, &old_child_name);
	int old_parent_inumber;

	char *new_parent_name, *new_child_name, newPath_copy[MAX_FILE_NAME];
	strcpy(newPath_copy, newPath);
	split_parent_child_from_path(newPath_copy, &new_parent_name, &new_child_name);
	int new_parent_inumber;

	if (flag) {
		old_parent_inumber = lookup(old_parent_name, activeLocks, &numActiveLocks, true);
		new_parent_inumber = lookup(new_parent_name, activeLocks, &numActiveLocks, true);
	} else {
		new_parent_inumber = lookup(new_parent_name, activeLocks, &numActiveLocks, true);
		old_parent_inumber = lookup(old_parent_name, activeLocks, &numActiveLocks, true);
	}

	dir_add_entry(new_parent_inumber, inumber, new_child_name);
	dir_reset_entry(old_parent_inumber, inumber);
	unlockAll(activeLocks, numActiveLocks);
	return SUCCESS;
}
/*
int move(char* oldPath, char* newPath){
	int activeLocks[INODE_TABLE_SIZE], j = 0;
	



	int inumber = lookupMove(oldPath, activeLocks, &j, 1);
	printf("------ OLD CHILD INUMBER ------\n");
	int inumber = lookup(oldPath, activeLocks, &j, false);


	CHECK 
	if (inumber == FAIL) {
		printf("failed to move from %s to %s, there's no file or dir with the old path name\n", oldPath, newPath);
		unlockAll(activeLocks, j);
		return FAIL;
	}

	unlockAll(activeLocks, j);
	memset(activeLocks, 0, sizeof(activeLocks));
 	j=0;


	char *old_parent_name, *old_child_name, oldPath_copy[MAX_FILE_NAME];
	strcpy(oldPath_copy, oldPath);
	split_parent_child_from_path(oldPath_copy, &old_parent_name, &old_child_name);

	printf("------ OLD PARENT INUMBER ------\n");

	int old_parent_inumber = lookup(old_parent_name, activeLocks, &j, true);

	char *new_parent_name, *new_child_name, newPath_copy[MAX_FILE_NAME];
	strcpy(newPath_copy, newPath);
	split_parent_child_from_path(newPath_copy, &new_parent_name, &new_child_name);

	printf("------ NEW PARENT INUMBER ------\n");
	int new_parent_inumber = lookup(new_parent_name, activeLocks, &j, false);

	unlockAll(activeLocks, j);
	memset(activeLocks, 0, sizeof(activeLocks));
 	j=0;
	
	CHECK 
	if (new_parent_inumber == FAIL) {
		printf("failed to move from %s to %s, there's no file or dir with the new parent name\n", oldPath, newPath);
		unlockAll(activeLocks, j);
		return FAIL;
	}

	union Data new_pdata;
	type new_pType;
	inode_get(new_parent_inumber, &new_pType, &new_pdata);

	CHECK
	if (new_pType != T_DIRECTORY) {
		printf("failed to move from %s to %s, there's a file with the new parent name, not a dir\n", oldPath, newPath);
		unlockAll(activeLocks, j);
		return FAIL;
	}

	CHECK
	printf("------ NEW CHILD ------\n");
	if (lookup(newPath, activeLocks, &j, false) != FAIL){
		printf("failed to move from %s to %s, there's a file or dir with the new path name\n", oldPath, newPath);
		unlockAll(activeLocks, j);
		return FAIL;
	}

	dir_add_entry(new_parent_inumber, inumber, new_child_name);
	dir_reset_entry(old_parent_inumber, inumber);

	unlockAll(activeLocks, j);
	return SUCCESS;
}
*/

/*
 * Prints tecnicofs tree.
 * Input:
 *  - fp: pointer to output file
 */
void print_tecnicofs_tree(FILE *fp){
	inode_print_tree(fp, FS_ROOT, "");
}

