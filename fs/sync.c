#include "sync.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Locks the RWLock (read mode) or Mutex depending on the synchronisation strategy.
void lockRead(){
	if (syncStrategy == RWLOCK) {
		if (pthread_rwlock_rdlock(&rwl) != 0) {
			fprintf(stderr, "Error locking with lockRead() with RWLOCK!\n");
			exit(EXIT_FAILURE);
		}
	}
	else if (syncStrategy == MUTEX) {
		if(pthread_mutex_lock(&mutex) != 0) {
			fprintf(stderr, "Error locking with lockRead() with MUTEX!\n");
			exit(EXIT_FAILURE);		
		}
	}
}

// Locks the RWLock (read+write mode) or Mutex depending on the synchronisation strategy.
void lockWrite(){
	if (syncStrategy == RWLOCK) {
		if (pthread_rwlock_wrlock(&rwl) != 0) {
			fprintf(stderr, "Error locking with lockWrite() with RWLOCK!\n");
			exit(EXIT_FAILURE);
		}
	}
	else if (syncStrategy == MUTEX) {
		if (pthread_mutex_lock(&mutex) != 0) {
			fprintf(stderr, "Error locking with lockWrite() with MUTEX!\n");
			exit(EXIT_FAILURE);
		}
	}
}

// Unlocks the RWLock or Mutex depending on the synchronization strategy.
void unlock(){
	if (syncStrategy == RWLOCK) {
		if (pthread_rwlock_unlock(&rwl) != 0) {
			fprintf(stderr, "Error unlocking with unlock() using RWLOCK!\n");
			exit(EXIT_FAILURE);
		}
	}

	else if (syncStrategy == MUTEX) {
		if (pthread_mutex_unlock(&mutex) != 0) {
			fprintf(stderr, "Error unlocking with unlock() using MUTEX!\n");
			exit(EXIT_FAILURE);
		}
	}
}

// Destroys the synchronization structures
void destroySyncStructures(){
	if (syncStrategy == MUTEX) {
    	if (pthread_mutex_destroy(&mutex) != 0) {
			fprintf(stderr, "Error destroying MUTEX!\n");
			exit(EXIT_FAILURE);
		}
		if (pthread_mutex_destroy(&main_mutex) != 0) {
			fprintf(stderr, "Error destroying command vector MUTEX!\n");
			exit(EXIT_FAILURE);
		}
	}
    if (syncStrategy == RWLOCK) {
        if (pthread_rwlock_destroy(&rwl) != 0) {
			fprintf(stderr, "Error destroying RW LOCK!\n");
			exit(EXIT_FAILURE);
		}
		if (pthread_mutex_destroy(&main_mutex) != 0) {
			fprintf(stderr, "Error destroying command vector MUTEX!\n");
			exit(EXIT_FAILURE);
		}
	}
}
