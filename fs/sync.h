#ifndef __SYNC_H__
#define __SYNC_H__

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../tecnicofs-api-constants.h"

#define NOSYNC 0
#define MUTEX 1
#define RWLOCK 2

#define TECNICOFS_ERROR_LOCK_READ -1
#define TECNICOFS_ERROR_LOCK_WRITE -2
#define TECNICOFS_ERROR_UNLOCK -3
#define TECNICOFS_ERROR_MUTEX_DESTROY -4
#define TECNICOFS_ERROR_RWLOCK_DESTROY -5

pthread_mutex_t mutex;
pthread_rwlock_t rwl;
int syncStrategy;

int lockRead();
int lockWrite();
int unlock();
int destroySyncStructures();

#endif