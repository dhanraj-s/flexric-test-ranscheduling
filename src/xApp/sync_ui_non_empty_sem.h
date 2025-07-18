#ifndef SYNC_UI_NON_EMPTY_SEM_H
#define SYNC_UI_NON_EMPTY_SEM_H

#include <semaphore.h>
#include <pthread.h>

extern sem_t sync_ui_non_empty_sem;
extern pthread_mutex_t sync_ui_non_empty_mutex;

#endif
