#include <stdio.h> 
#include <pthread.h> 
#include <stdlib.h>

#include "barrier.h"

// -------------------------------------------------------------------------------- //

int barrier_init(barrier_t *barrier, uint8_t threshold) {
    if (pthread_mutex_init(&barrier->mutex, NULL) != 0) {
        printf("Error: pthread_mutex_init in %s\n", __func__);
        return -1;
    }
    
    if (pthread_cond_init(&barrier->cond, NULL) != 0) {
        pthread_mutex_destroy(&barrier->mutex);
        printf("Error: pthread_cond_init in %s\n", __func__);
        return -1;
    }

    barrier->waiting_threads = 0;
    barrier->threshold = threshold;

    return 0;
}

// -------------------------------------------------------------------------------- //

void barrier_wait(barrier_t *barrier) {
    pthread_mutex_lock(&barrier->mutex);
    barrier->waiting_threads++;
    if (barrier->waiting_threads >= barrier->threshold) {
        barrier->waiting_threads = 0;
        pthread_cond_broadcast(&barrier->cond);
    } else {
        pthread_cond_wait(&barrier->cond, &barrier->mutex);
    }
    pthread_mutex_unlock(&barrier->mutex);
}

// -------------------------------------------------------------------------------- //

void barrier_release(barrier_t *barrier) {
    pthread_mutex_lock(&barrier->mutex);
    pthread_cond_broadcast(&barrier->cond);
    pthread_mutex_unlock(&barrier->mutex);
}

// -------------------------------------------------------------------------------- //

void barrier_destroy(barrier_t *barrier) {
    pthread_mutex_destroy(&barrier->mutex);
    pthread_cond_destroy(&barrier->cond);
}

// -------------------------------------------------------------------------------- //
