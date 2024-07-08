#ifndef BARRIER_H
#define BARRIER_H

// -------------------------------------------------------------------------------- //

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

// -------------------------------------------------------------------------------- //

typedef struct barrier_t {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    uint8_t waiting_threads;
    uint8_t threshold;
} barrier_t;

// -------------------------------------------------------------------------------- //

int barrier_init(barrier_t *barrier, uint8_t threshold);
void barrier_wait(barrier_t *barrier);
void barrier_release(barrier_t *barrier);
void barrier_destroy(barrier_t *barrier);

// -------------------------------------------------------------------------------- //

#endif // BARRIER_H
