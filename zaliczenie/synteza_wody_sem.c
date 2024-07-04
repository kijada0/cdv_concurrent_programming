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

#define ITERATIONS_COUNT            10 // water molecules to generate
#define PRODUCERS_COUNT             5 

#define SHERED_MEMORY_NAME          "/kk_mem_water"

#define SEMAPHORE_NAME_GENERATOR    "/kk_sem_generator"
#define SEMAPHORE_NAME_PRODUCE      "/kk_sem_produce"

// -------------------------------------------------------------------------------- //

void sleep_rand() {
    int sleep_time = 100 + rand() % (1000 - 100);
    printf("Sleeping for %d ms\n", sleep_time);
    usleep(sleep_time * 1000);
}

// -------------------------------------------------------------------------------- //

void water_generator(char *producers, uint8_t *runing) {
    int i, j;
    int *address;
    sem_t *sem_gen, **sem_prod;
    char sem_name[50];
    char elements_buffer[PRODUCERS_COUNT + 1];
    int safety_counter = 100;
    uint8_t condition = 0;

    // init generator semaphore
    sem_gen = sem_open(SEMAPHORE_NAME_GENERATOR, O_CREAT, 0600, 1);
    if(sem_gen == SEM_FAILED) {
        perror("Problem with opening semaphore in water generator");
        exit(1);
    }

    // init producer semaphores
    sem_prod = malloc(sizeof(sem_t *) * PRODUCERS_COUNT);
    if (sem_prod == NULL) {
        perror("Failed to allocate memory for sem_prod");
        exit(1);
    }
    for (i = 0; i < PRODUCERS_COUNT; i++) {
        sprintf(sem_name, "%s_%d", SEMAPHORE_NAME_PRODUCE, i);
        sem_prod[i] = sem_open(sem_name, O_CREAT, 0600, 1);
        if (sem_prod[i] == SEM_FAILED) {
            perror("Failed to open semaphore in water generator");
            exit(1);
        }
    }

    printf("Start water generator (PID: %d) (func: %s)\n", getpid(), __func__);

    while(safety_counter && *runing) {
        memcpy(elements_buffer, producers, PRODUCERS_COUNT);
        elements_buffer[PRODUCERS_COUNT] = '\0';
        printf("Water molecule: %s\n", elements_buffer);

        condition = 1;
        for (j = 0; j < PRODUCERS_COUNT; j++) {
            if (producers[j] != 'H') {
                condition = 0;
                printf("Waiting for producer no. %d\n", j);
                producers[j] = '-';
                sem_post(sem_prod[j]);
                sem_wait(sem_gen);
            }
        }

        if (condition == 1) {
            printf("Water molecule created\n");
            break;
        }

        safety_counter--;
    }

    memcpy(elements_buffer, producers, PRODUCERS_COUNT);
    elements_buffer[PRODUCERS_COUNT] = '\0';
    printf("Water molecule: %s\n", elements_buffer);

    // signal producers to end
    *runing = 0;
    for (int j = 0; j < PRODUCERS_COUNT; j++) {
        sem_post(sem_prod[j]);
    }

    free(sem_prod);
    printf("><\tEnd of water generator\n");
}

// -------------------------------------------------------------------------------- //

void elements_producer(char *producers, uint8_t producer_id, uint8_t *runing) {
    int i = 0, r;
    int *address;
    sem_t *sem_gen, **sem_prod;
    char sem_name[50];
    char elements_buffer[PRODUCERS_COUNT + 1];
    srand(time(NULL) + producer_id);

    // init generator semaphore
    sem_gen = sem_open(SEMAPHORE_NAME_GENERATOR, O_CREAT, 0600, 0);
    if(sem_gen == SEM_FAILED) {
        perror("Problem with opening semaphore in water generator");
        exit(1);
    }

    // init producer semaphores
    sem_prod = malloc(sizeof(sem_t *) * PRODUCERS_COUNT);
    if (sem_prod == NULL) {
        perror("Failed to allocate memory for sem_prod");
        exit(1);
    }
    for (i = 0; i < PRODUCERS_COUNT; i++) {
        sprintf(sem_name, "%s_%d", SEMAPHORE_NAME_PRODUCE, i);
        sem_prod[i] = sem_open(sem_name, O_CREAT, 0600, 1);
        if (sem_prod[i] == SEM_FAILED) {
            perror("Failed to open semaphore in water generator");
            exit(1);
        }
    }

    printf("Start prfucer no. %d (PID: %d) (func: %s)\n", producer_id, getpid(), __func__);

    sem_wait(sem_prod[producer_id]);

    while(*runing) {
        sem_wait(sem_prod[producer_id]);
        if(!*runing) {
            break;
        }

        r = rand() % 2;
        if(r == 0) {
            producers[producer_id] = 'H';
        } else {
            producers[producer_id] = 'O';
        }
        printf("\t\t\tProducer no. %d: %c\n", producer_id, producers[producer_id]);

        sleep_rand();
        sem_post(sem_gen);
    }

    free(sem_prod);
    printf("<>\tEnd of producer no. %d\n", producer_id);
}

// -------------------------------------------------------------------------------- //

int main() {
    int shered_memory_id;
    uint8_t *shared_memory_printer;
    char *producers_list;
    uint8_t *runing;
    int pid;
    int process_pid_list[PRODUCERS_COUNT];
    int i;

    srand(time(NULL));

    printf("Starting water synthesis (%s)\n", __FILE__);

    // Clean up after previous run
    shm_unlink(SHERED_MEMORY_NAME);
    sem_unlink(SEMAPHORE_NAME_GENERATOR);
    sem_unlink(SEMAPHORE_NAME_PRODUCE);

    // init shared memory
    printf("Init shared memory\n");
    shered_memory_id = shm_open(SHERED_MEMORY_NAME, O_CREAT | O_RDWR, 0600);
    if(ftruncate(shered_memory_id, PRODUCERS_COUNT*sizeof(char)) != 0) {
        perror("ftruncate");
        exit(1);
    }
    shared_memory_printer = mmap(NULL, PRODUCERS_COUNT*sizeof(char) + sizeof(uint8_t), PROT_READ | PROT_WRITE, MAP_SHARED, shered_memory_id, 0);

    // Producers init
    producers_list = (char *)shared_memory_printer;
    for (i = 0; i < PRODUCERS_COUNT; i++) {
        producers_list[i] = '-';
    }

    // Runing flag init
    runing = (uint8_t *)&shared_memory_printer[PRODUCERS_COUNT*sizeof(char)];
    *runing = 1;

    // Create child processes
    printf("\nSpawning producers... \n");
    for (i = 0; i < PRODUCERS_COUNT; i++) {
        pid = fork();
        if(pid == 0) {
            elements_producer(producers_list, i, runing);
            return 0;
        }
        process_pid_list[i] = pid;
    }
    sleep(1); // wait for producers to start
    
    // Start water generator
    water_generator(producers_list, runing);

    // Clean up after child processes
    sleep(2); // wait for producers to end
    printf("\nClosing water synthesis... \n");
    for (i = 0; i < PRODUCERS_COUNT; i++) {
        printf("Killing process no. %d (PID: %d)\n", i, process_pid_list[i]);
        kill(process_pid_list[i], SIGKILL);
    }

    // Clean up after main process
    munmap(shared_memory_printer, PRODUCERS_COUNT*sizeof(char));
    shm_unlink(SHERED_MEMORY_NAME);
    sem_unlink(SEMAPHORE_NAME_GENERATOR);
    sem_unlink(SEMAPHORE_NAME_PRODUCE);

    return 0;
}
