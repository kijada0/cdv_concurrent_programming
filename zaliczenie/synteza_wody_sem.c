#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <semaphore.h>
#include <pthread.h>

// -------------------------------------------------------------------------------- //

#define ITERATIONS_COUNT            100 // water molecules to generate
#define PRODUCERS_COUNT             5

#define SHERED_MEMORY_NAME          "/kk_mem_water"

#define SEMAPHORE_NAME_GENERATOR    "/kk_sem_generator"
#define SEMAPHORE_NAME_PRODUCE      "/kk_sem_produce"

// -------------------------------------------------------------------------------- //

void water_generator(char *producers) {
    int i = 0;
    int *address;
    sem_t *sem_gen, *sem_prod;
    char elements_buffer[PRODUCERS_COUNT + 1];

    sem_gen = sem_open(SEMAPHORE_NAME_GENERATOR, O_CREAT, 0600, PRODUCERS_COUNT);
    sem_prod = sem_open(SEMAPHORE_NAME_PRODUCE, O_CREAT, 0600, 0);

    if(sem_gen == SEM_FAILED || sem_prod == SEM_FAILED) {
        perror("Problem with opening semaphore in water generator");
        exit(1);
    }

    printf("Start water generator (PID: %d) (func: %s)\n", getpid(), __func__);

    for(i = 0; i < ITERATIONS_COUNT; i++) {
        sem_wait(sem_gen);

        memcpy(elements_buffer, producers, PRODUCERS_COUNT);
        elements_buffer[PRODUCERS_COUNT] = '\0';
        printf("Water molecule: %s\n", elements_buffer);

        sem_post(sem_prod);
    }

    printf("End of water generator\n");
}

// -------------------------------------------------------------------------------- //

void elements_producer(char *producers, uint8_t producer_id) {
    int i = 0;
    int *address;
    sem_t *sem_gen, *sem_prod;

    sem_gen = sem_open(SEMAPHORE_NAME_GENERATOR, O_CREAT, 0600, PRODUCERS_COUNT);
    sem_prod = sem_open(SEMAPHORE_NAME_PRODUCE, O_CREAT, 0600, 0);

    if(sem_gen == SEM_FAILED || sem_prod == SEM_FAILED) {
        perror("Problem with opening semaphore in hydrogen producer");
        exit(1);
    }

    printf("Start prfucer no. %d (PID: %d) (func: %s)\n", producer_id, getpid(), __func__);

    for(i = 0; i < ITERATIONS_COUNT; i++) {
        sem_wait(sem_gen);

        printf("Producer no. %d: H\n", producer_id);
        producers[producer_id] = 'H';

        sem_post(sem_prod);
    }

    printf("End of producer no. %d\n", producer_id);
}

// -------------------------------------------------------------------------------- //

int main() {
    int shered_memory_id;
    char *shared_memory_printer;
    int pid;
    int process_pid_list[PRODUCERS_COUNT];
    int i;

    printf("Starting water synthesis (%s)\n", __FILE__);

    // init shared memory
    printf("Init shared memory\n");
    shered_memory_id = shm_open(SHERED_MEMORY_NAME, O_CREAT | O_RDWR, 0600);
    if(ftruncate(shered_memory_id, PRODUCERS_COUNT*sizeof(char)) != 0) {
        perror("ftruncate");
        exit(1);
    }
    shared_memory_printer = mmap(NULL, PRODUCERS_COUNT*sizeof(char), PROT_READ | PROT_WRITE, MAP_SHARED, shered_memory_id, 0);

    // Producers init
    for (i = 0; i < PRODUCERS_COUNT; i++) {
        shared_memory_printer[i] = '\0';
    }

    // create processes
    pid = fork();
    if(pid == 0) {
        water_generator(shared_memory_printer);
        goto end;
    }

    for (i = 0; i < PRODUCERS_COUNT; i++) {
        pid = fork();
        if(pid == 0) {
            process_pid_list[i] = getpid();
            elements_producer(shared_memory_printer, i);
            return 0;
        }
    }
    printf("End of main process\n");
    return 0;

    end:
    printf("End generator\n");

    for (i = 0; i < PRODUCERS_COUNT; i++) {
        kill(process_pid_list[i], SIGKILL);
    }

    // Clean up
    munmap(shared_memory_printer, PRODUCERS_COUNT*sizeof(char));
    shm_unlink(SHERED_MEMORY_NAME);

    return 0;
}
