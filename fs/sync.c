#include "sync.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Locks the RWLock (read mode) or Mutex depending on the synchronisation strategy.
int lockRead(){
	if (syncStrategy == RWLOCK) {
		if (pthread_rwlock_rdlock(&rwl) != 0) {
			fprintf(stderr, "Error locking with lockRead() with RWLOCK!\n");
			return TECNICOFS_ERROR_LOCK_READ;
		}
	}
	else if (syncStrategy == MUTEX) {
		if(pthread_mutex_lock(&mutex) != 0) {
			fprintf(stderr, "Error locking with lockRead() with MUTEX!\n");
			return TECNICOFS_ERROR_LOCK_READ;
		}
	}
	return EXIT_SUCCESS;
}

// Locks the RWLock (read+write mode) or Mutex depending on the synchronisation strategy.
int lockWrite(){
	if (syncStrategy == RWLOCK) {
		if (pthread_rwlock_wrlock(&rwl) != 0) {
			fprintf(stderr, "Error locking with lockWrite() with RWLOCK!\n");
			return TECNICOFS_ERROR_LOCK_WRITE;
		}
	}
	else if (syncStrategy == MUTEX) {
		if (pthread_mutex_lock(&mutex) != 0) {
			fprintf(stderr, "Error locking with lockWrite() with MUTEX!\n");
			return TECNICOFS_ERROR_LOCK_WRITE;
		}
	}
	return EXIT_SUCCESS;
}

// Unlocks the RWLock or Mutex depending on the synchronization strategy.
int unlock(){
	if (syncStrategy == RWLOCK) {
		if (pthread_rwlock_unlock(&rwl) != 0) {
			fprintf(stderr, "Error unlocking with unlock() with RWLOCK!\n");
			return TECNICOFS_ERROR_UNLOCK;
		}
	}

	else if (syncStrategy == MUTEX) {
		if (pthread_mutex_unlock(&mutex) != 0) {
			fprintf(stderr, "Error unlocking with unlock() with MUTEX!\n");
			return TECNICOFS_ERROR_UNLOCK;
		}
	}
	return EXIT_SUCCESS;
}

// Destroys the synchronization structures
int destroySyncStructures(){
    if (pthread_mutex_destroy(&mutex) != 0) {
		fprintf(stderr, "Error destroying MUTEX!\n");
		return TECNICOFS_ERROR_MUTEX_DESTROY;
	}
    if (syncStrategy == RWLOCK) {
        if (pthread_rwlock_destroy(&rwl) != 0) {
			fprintf(stderr, "Error destroying MUTEX!\n");
			return TECNICOFS_ERROR_RWLOCK_DESTROY;
		}
	}
	return EXIT_SUCCESS;
}