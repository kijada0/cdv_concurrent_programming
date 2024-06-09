#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <semaphore.h>

// -------------------------------------------------------------------------------- //
#define BUFFER_SIZE 5
#define ITERATIONS_COUNT 20

#define SHERED_MEMORY_SIZE (BUFFER_SIZE * sizeof(int))
#define SHERED_MEMORY_NAME "/kk_mem"

#define SEMAPHORE_NAME_W "/kk_sem_writer"
#define SEMAPHORE_NAME_R "/kk_sem_reader"

// -------------------------------------------------------------------------------- //

void parent_process(int *shared_memory_printer) {
    int i = 0;
    int *address;

    sem_t *semaphore_w, *semaphore_r;
    semaphore_w = sem_open(SEMAPHORE_NAME_W, O_CREAT, 0600, 5);
    semaphore_r = sem_open(SEMAPHORE_NAME_R, O_CREAT, 0600, 0);

    if(semaphore_w == SEM_FAILED || semaphore_r == SEM_FAILED) {
        perror("Parent: sem_open");
        exit(1);
    }

    printf("Parent: Start\n");

    for(i = 1; i < ITERATIONS_COUNT; i++) {
        sem_wait(semaphore_w);
        address = &shared_memory_printer[i*BUFFER_SIZE];
        *address = i;
        sem_post(semaphore_r);

        printf("Parent: %d\n", i);
    }
}

// -------------------------------------------------------------------------------- //

void child_process(int *shared_memory_printer) {
    int i = 0;
    int n = 0;
    int *address;

    sem_t *semaphore_w, *semaphore_r;
    semaphore_w = sem_open(SEMAPHORE_NAME_W, O_CREAT, 0600, 5);
    semaphore_r = sem_open(SEMAPHORE_NAME_R, O_CREAT, 0600, 0);

    if(semaphore_w == SEM_FAILED || semaphore_r == SEM_FAILED) {
        perror("Child: sem_open");
        exit(1);
    }

    printf("Child: Start\n");

    for(i = 1; i < ITERATIONS_COUNT; i++) {
        sem_wait(semaphore_r);
        address = &shared_memory_printer[i*BUFFER_SIZE];
        n = *address;
        sem_post(semaphore_w);

        printf("\t\tChild: %d\n", n);
    }
}

// -------------------------------------------------------------------------------- //

int main() {
    int shered_memory_id;
    int *shared_memory_printer;
    int pid;

    printf("Start: %s\n", __FILE__);

    // init shared memory
    printf("Init shared memory\n");
    shered_memory_id = shm_open(SHERED_MEMORY_NAME, O_CREAT | O_RDWR, 0600);
    ftruncate(shered_memory_id, SHERED_MEMORY_SIZE);
    shared_memory_printer = mmap(NULL, SHERED_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shered_memory_id, 0);

    *shared_memory_printer = 0;

    // fork
    printf("\nFork\n");
    pid = fork();
    if(pid == 0) {
        child_process(shared_memory_printer);
        printf("\nChild: Exit\n");
        return 0;
    }
    else {
        parent_process(shared_memory_printer);
        wait(NULL);
    }

    // cleanup
    printf("\nCleanup\n");
    shm_unlink(SHERED_MEMORY_NAME);
    sem_unlink(SEMAPHORE_NAME_W);
    sem_unlink(SEMAPHORE_NAME_R);

    return 0;
}
