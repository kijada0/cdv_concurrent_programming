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

#define MOLECULES_TO_CREATE         10
#define PRODUCERS_COUNT             8

#define SHERED_MEMORY_NAME          "/kk_mem_water"

#define SEMAPHORE_NAME_GENERATOR    "/kk_sem_generator"
#define SEMAPHORE_NAME_PRODUCE      "/kk_sem_produce"

// -------------------------------------------------------------------------------- //

void random_sleep() {
    int sleep_time = 1000 + rand() % 2000;
    printf("sleep(%d)\n", sleep_time);
    usleep(sleep_time * 1000);
}

uint8_t count_water_molecules(uint8_t h, uint8_t o) {
    uint8_t water_molecules_from_hydrogen = h / 2;
    uint8_t water_molecules_from_oxygen = o;
    
    if (water_molecules_from_hydrogen < water_molecules_from_oxygen) {
        return water_molecules_from_hydrogen;
    } else {
        return water_molecules_from_oxygen;
    }
}

// -------------------------------------------------------------------------------- //

void water_generator(char *producers, uint8_t *running) {
    int i, j;
    sem_t *sem_gen, **sem_prod;
    char sem_name[50];
    char elements_buffer[PRODUCERS_COUNT + 1];
    int safety_counter = 100;
    uint8_t thread_running;

    uint8_t generated_water_molecules = 0;
    uint8_t posible_water_molecules;
    uint8_t hydrogen_count, oxygen_count;
    
    // Init generator semaphore
    sem_gen = sem_open(SEMAPHORE_NAME_GENERATOR, O_CREAT, 0600, 0);
    if(sem_gen == SEM_FAILED) {
        perror("Problem with opening semaphore in water generator");
        exit(1);
    }

    // Init producer semaphores
    sem_prod = malloc(sizeof(sem_t *) * PRODUCERS_COUNT);
    if (sem_prod == NULL) {
        perror("Failed to allocate memory for sem_prod");
        exit(1);
    }
    for (i = 0; i < PRODUCERS_COUNT; i++) {
        snprintf(sem_name, sizeof(sem_name), "%s_%d", SEMAPHORE_NAME_PRODUCE, i);
        sem_prod[i] = sem_open(sem_name, O_CREAT, 0600, 1);
        if (sem_prod[i] == SEM_FAILED) {
            perror("Failed to open semaphore in water generator");
            exit(1);
        }
    }

    printf("Start water generator (PID: %d) (func: %s)\n", getpid(), __func__);

    while(safety_counter && *running && generated_water_molecules < MOLECULES_TO_CREATE) {
        memcpy(elements_buffer, producers, PRODUCERS_COUNT);
        elements_buffer[PRODUCERS_COUNT] = '\0';
        printf("\nWater molecule: %s\n", elements_buffer);
        thread_running = 0;
        
        // Check if there are enough elements to create water molecule
        hydrogen_count = 0;
        oxygen_count = 0;
        for (j = 0; j < PRODUCERS_COUNT; j++) {
            if (producers[j] == 'H') {
                hydrogen_count++;
            } else if (producers[j] == 'O') {
                oxygen_count++;
            }
        }

        // Create water molecule if there are enough elements
        posible_water_molecules = count_water_molecules(hydrogen_count, oxygen_count);
        if (posible_water_molecules > 0) {
            generated_water_molecules += posible_water_molecules;
            hydrogen_count = posible_water_molecules * 2;
            oxygen_count = posible_water_molecules;

            printf("# Water molecule created (%d) (new: %d)\n", generated_water_molecules, posible_water_molecules);

            // Remove elements from producers
            for (j = 0; j < PRODUCERS_COUNT; j++) {
                if (producers[j] == 'H') {
                    printf("!\tClearing producer no. %d (H)\n", j);
                    producers[j] = '-';
                    hydrogen_count--;
                    sem_post(sem_prod[j]);
                    thread_running++;
                    if (hydrogen_count == 0) {
                        break;
                    }
                }
            }
            for (j = 0; j < PRODUCERS_COUNT; j++) {
                if (producers[j] == 'O') {
                    printf("!\tClearing producer no. %d (O)\n", j);
                    producers[j] = '-';
                    oxygen_count--;
                    sem_post(sem_prod[j]);
                    thread_running++;
                    if (oxygen_count == 0) {
                        break;
                    }
                }
            }

            // Clear random elements
            for (j = 0; j < PRODUCERS_COUNT/3; j++) {
                i = rand() % PRODUCERS_COUNT;
                if (producers[i] != '-') {
                    printf("!!\tClearing producer no. %d (random)\n", i);
                    producers[i] = '-';
                    sem_post(sem_prod[i]);
                    thread_running++;
                }
            }
        }
        else {
            printf("# Not enough elements to make water!\n");

            // Clear elements
            for (j = 0; j < PRODUCERS_COUNT; j++) {
                printf("!!\tClearing producer no. %d\n", j);
                producers[j] = '-';
                sem_post(sem_prod[j]);
                thread_running++;
            }
        }

        for (j = 0; j < thread_running; j++) {
            sem_wait(sem_gen);
        }

        printf(">\tIteration end\n");
        safety_counter--;
    }

    printf("Safety counter: %d\n", safety_counter);
    printf("Generated water molecules: %d\n", generated_water_molecules);

    memcpy(elements_buffer, producers, PRODUCERS_COUNT);
    elements_buffer[PRODUCERS_COUNT] = '\0';
    printf("Water molecule: %s\n", elements_buffer);

    // Signal producers to end
    *running = 0;
    for (j = 0; j < PRODUCERS_COUNT; j++) {
        sem_post(sem_prod[j]);
    }

    for (i = 0; i < PRODUCERS_COUNT; i++) {
        sem_close(sem_prod[i]);
        snprintf(sem_name, sizeof(sem_name), "%s_%d", SEMAPHORE_NAME_PRODUCE, i);
        sem_unlink(sem_name);
    }

    sem_close(sem_gen);
    sem_unlink(SEMAPHORE_NAME_GENERATOR);
    free(sem_prod);

    printf("><\tEnd of water generator\n");
}

// -------------------------------------------------------------------------------- //

void elements_producer(char *container, uint8_t producer_id, uint8_t *running) {
    int r;
    sem_t *sem_gen, *sem_prod;
    char sem_name[50];

    srand(time(NULL) + producer_id);

    // Init generator semaphore
    sem_gen = sem_open(SEMAPHORE_NAME_GENERATOR, O_CREAT, 0600, 0);

    // Init producer semaphores
    snprintf(sem_name, sizeof(sem_name), "%s_%d", SEMAPHORE_NAME_PRODUCE, producer_id);
    sem_prod = sem_open(sem_name, O_CREAT, 0600, 0);

    if (sem_prod == SEM_FAILED || sem_gen == SEM_FAILED) {
        perror("Failed to open semaphore in water generator");
        exit(1);
    }

    printf("Start producer no. %d (PID: %d) (func: %s)\n", producer_id, getpid(), __func__);

    while(*running) {
        printf("~\t\t\t\t\t\tWaiting for generator %d\n", producer_id);
        sem_wait(sem_prod);
        if(!*running) {
            break;
        }
        
        random_sleep();

        r = rand() % 2;
        if(r == 0) {
            *container = 'H';
        } else {
            *container = 'O';
        }
        printf(">\t\t\t\t\t\tProducer no. %d: %c\n", producer_id, *container);

        sem_post(sem_gen);
    }

    sem_close(sem_gen);
    sem_unlink(SEMAPHORE_NAME_GENERATOR);
    sem_close(sem_prod);
    sem_unlink(sem_name);

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
    printf("----------------------------------------------------------------------------------------------------\n");
    printf("Starting water synthesis (%s)\n", __FILE__);

    // Clean up after previous run
    shm_unlink(SHERED_MEMORY_NAME);
    sem_unlink(SEMAPHORE_NAME_GENERATOR);

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
            elements_producer(&producers_list[i], i, runing);
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

    printf("--------------------------------------------------------------------------------\n");

    return 0;
}
