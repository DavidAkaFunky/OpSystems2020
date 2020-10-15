#ifndef __SYNC_H__
#define __SYNC_H__

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../tecnicofs-api-constants.h"

#define NOSYNC 0
#define MUTEX 1
#define RWLOCK 2

pthread_mutex_t mutex;
pthread_rwlock_t rwl;
int syncStrategy;

void lockRead();
void lockWrite();
void unlock();
void destroySyncStructures();

#endif