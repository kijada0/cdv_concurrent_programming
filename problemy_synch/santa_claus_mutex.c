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
#include <pthread.h>

// -------------------------------------------------------------------------------- //

#define RENIFER_COUNT       5
#define RENIFER_TRESHOLD    3

#define ELF_COUNT           5
#define ELF_TRESHOLD        3

#define ITERATIONS_COUNT    5 // Renifer Treshold Amount
#define SANTA_WAIT_LIMIT    25

// -------------------------------------------------------------------------------- //

typedef struct {
    int id;
    uint8_t *runing;
    uint8_t *elf_waiting;
    uint8_t *renifer_waiting;
    pthread_mutex_t *mutex;
    pthread_mutex_t *mutex_cond;
    pthread_mutex_t *mutex_bussy;
    pthread_cond_t *condition_client;
    pthread_cond_t *condition_santa;
    pthread_barrier_t *barrier;
} thread_data_t;

typedef struct {
    uint8_t *runing;
    uint8_t *elf_waiting;
    uint8_t *renifer_waiting;
    pthread_mutex_t *mutex;
    pthread_mutex_t *mutex_cond;
    pthread_mutex_t *mutex_bussy;
    pthread_cond_t *condition_client;
    pthread_cond_t *condition_santa;
} santa_data_t;

// -------------------------------------------------------------------------------- //

int get_random(int min, int max) {
    return min + rand() % (max - min);
}

// -------------------------------------------------------------------------------- //

void santa_process(santa_data_t *params) {
    printf("\n--------------------------------------------------------------------------------\n");
    printf("Santa: Start\n");
    pthread_mutex_t *mutex = params->mutex;
    uint8_t completed_iterations = 0;
    uint8_t r_count = 0, e_count = 0;
    uint8_t safety_counter;

    while(1) {
        // wait for signal
        printf("Santa: Waiting for signal\n");
        pthread_cond_wait(params->condition_santa, params->mutex_cond);
        printf("Santa: Wake up\n");
        pthread_mutex_lock(params->mutex_bussy);
        
        // check who wake up
        pthread_mutex_lock(mutex);
        r_count = *params->renifer_waiting;
        e_count = *params->elf_waiting;
        printf("Santa: Renifers: %d, Elfs: %d\n", r_count, e_count);
        pthread_mutex_unlock(mutex);

        sleep(1);

        pthread_mutex_lock(mutex);
        if(*params->renifer_waiting >= RENIFER_TRESHOLD) {
            pthread_mutex_unlock(mutex);

            printf("Santa: Help Renifers\n");
            pthread_cond_broadcast(params->condition_client);
            pthread_mutex_unlock(params->mutex_cond);
            safety_counter = 0;

            while(1) {
                printf("Santa: Waiting for Renifers (%d)\n", safety_counter);

                pthread_mutex_lock(mutex);
                if(*params->renifer_waiting == 0) {
                    pthread_mutex_unlock(mutex);
                    break;
                }
                pthread_mutex_unlock(mutex);

                safety_counter++;
                if(safety_counter > SANTA_WAIT_LIMIT) {
                    perror("Santa: Safety counter exceeded");
                    break;
                }

                pthread_cond_broadcast(params->condition_client);
                sleep(1);
            }

            printf("Santa: End Help Renifers\n");
            completed_iterations++;
        }
        pthread_mutex_unlock(mutex);

        pthread_mutex_lock(mutex);
        if(*params->elf_waiting >= ELF_TRESHOLD) {
            pthread_mutex_unlock(mutex);

            printf("Santa: Help Elfs\n");
            pthread_cond_broadcast(params->condition_client);
            pthread_mutex_unlock(params->mutex_cond);
            safety_counter = 0;

            while(1) {
                printf("Santa: Waiting for Elfs (%d)\n", safety_counter);

                pthread_mutex_lock(mutex);
                if(*params->elf_waiting == 0) {
                    pthread_mutex_unlock(mutex);
                    break;
                }
                pthread_mutex_unlock(mutex);

                safety_counter++;
                if(safety_counter > SANTA_WAIT_LIMIT) {
                    printf("ERROR: Santa: Safety counter exceeded\n");
                    break;
                }
                
                pthread_cond_broadcast(params->condition_client);
                sleep(1);
            }

            printf("Santa: End Help Elfs\n");
            completed_iterations++;
        }
        pthread_mutex_unlock(mutex);
        
        pthread_mutex_unlock(params->mutex_cond);
        pthread_mutex_unlock(params->mutex_bussy);

        printf(">Santa: Completed iterations: <%d>\n", completed_iterations);
        if(completed_iterations >= ITERATIONS_COUNT) {
            break;
        }
    }

    pthread_mutex_unlock(params->mutex_bussy);

    // stop threads
    *params->runing = 0;

    pthread_mutex_unlock(mutex);

    printf("Santa: End\n");
}

// -------------------------------------------------------------------------------- //

void *renifer_process(void *args) {
    thread_data_t params;
    pthread_mutex_t *mutex;
    int r;

    memcpy(&params, (thread_data_t *)args, sizeof(thread_data_t));
    mutex = params.mutex;
    pthread_mutex_unlock(mutex);

    printf("\t\t\t\tRenifer (%d): Start\n", params.id);

    while(1) {
        pthread_mutex_lock(mutex);
        printf("\t\t\t\tRenifer (%d) runing: %d\n", params.id, *params.runing);
        if(*params.runing == 0) {
            pthread_mutex_unlock(mutex);
            pthread_barrier_wait(params.barrier);
            break;
        }
        pthread_mutex_unlock(mutex);

        pthread_mutex_lock(params.mutex_bussy);
        pthread_mutex_unlock(params.mutex_bussy);

        r = rand() % 5;
        if(r == 0) {
            printf("\t\t\t\tRenifer (%d): Waiting for Santa\n", params.id);
            pthread_mutex_lock(mutex);
            *params.renifer_waiting += 1;
            pthread_mutex_unlock(mutex);
            
            pthread_barrier_wait(params.barrier);

            // Check if Santa is runing
            pthread_mutex_lock(mutex);
            if(*params.runing == 0) {
                pthread_mutex_unlock(mutex);
                break;
            }
            pthread_mutex_unlock(mutex);

            printf("\t\t\t\tRenifer (%d): Wake up Santa...\n", params.id);
            pthread_cond_broadcast(params.condition_santa);

            pthread_cond_wait(params.condition_client, params.mutex_cond);
            printf("\t\t\t\tRenifer (%d): Runing with Santa...\n", params.id);

            pthread_mutex_lock(params.mutex);
            *params.renifer_waiting -= 1;
            printf("\t\t\t\tRenifer (%d): End Runing with Santa\n", params.id);
            pthread_mutex_unlock(params.mutex);

            pthread_mutex_unlock(params.mutex_cond);
        }

        usleep(get_random(100, 2500) * 1000);
    }

    printf("\t\t\t\tRenifer (%d): End\n", params.id);
}

// -------------------------------------------------------------------------------- //

void *elf_process(void *args) {
    thread_data_t params;
    pthread_mutex_t *mutex;
    int r;
    
    memcpy(&params, (thread_data_t *)args, sizeof(thread_data_t));
    mutex = params.mutex;
    pthread_mutex_unlock(mutex);

    printf("\t\t\t\t\t\t\t\tElf (%d): Start\n", params.id);

    while(1) {
        pthread_mutex_lock(mutex);
        printf("\t\t\t\t\t\t\t\tElf (%d): runing: %d\n", params.id, *params.runing);
        if(*params.runing == 0) {
            pthread_mutex_unlock(mutex);
            pthread_barrier_wait(params.barrier);
            break;
        }
        pthread_mutex_unlock(mutex);

        pthread_mutex_lock(params.mutex_bussy);
        pthread_mutex_unlock(params.mutex_bussy);

        r = rand() % 5;
        if(r == 0) {
            printf("\t\t\t\t\t\t\t\tElf (%d): Waiting for Santa\n", params.id);
            pthread_mutex_lock(mutex);
            *params.elf_waiting += 1;
            pthread_mutex_unlock(mutex);

            pthread_barrier_wait(params.barrier);

            // Check if Santa is runing
            pthread_mutex_lock(mutex);
            if(*params.runing == 0) {
                pthread_mutex_unlock(mutex);
                break;
            }
            pthread_mutex_unlock(mutex);

            printf("\t\t\t\t\t\t\t\tElf (%d): Wake up Santa...\n", params.id);
            pthread_cond_broadcast(params.condition_santa);

            pthread_cond_wait(params.condition_client, params.mutex_cond);
            printf("\t\t\t\t\t\t\t\tElf (%d): Runing with Santa...\n", params.id);

            pthread_mutex_lock(params.mutex);
            *params.elf_waiting -= 1;
            printf("\t\t\t\t\t\t\t\tElf (%d): End Runing with Santa\n", params.id);
            pthread_mutex_unlock(params.mutex);

            pthread_mutex_unlock(params.mutex_cond);
        }

        usleep(get_random(100, 2500) * 1000);
    }

    printf("\t\t\t\t\t\t\t\tElf (%d): End\n", params.id);
}

// -------------------------------------------------------------------------------- //

int main() {
    int i;
    uint8_t runing = 1;
    uint8_t elf_waiting = 0, renifer_waiting = 0;

    santa_data_t santa_args;
    thread_data_t thread_args;
    pthread_t thread_tab_renifer[RENIFER_COUNT];
    pthread_t thread_tab_elf[ELF_COUNT];

    pthread_mutex_t mutex;
    pthread_mutex_t mutex_cond;
    pthread_mutex_t mutex_bussy;
    pthread_cond_t condition_client;
    pthread_cond_t condition_santa;
    pthread_barrier_t barrier_renifer;
    pthread_barrier_t barrier_elf;

    // ---------------------------------------- //

    printf("Start: %s\n", __FILE__);

    // init random
    srand(time(NULL));

    // ---------------------------------------- //

    // init mutex
    printf("Init mutex\n");
    if(pthread_mutex_init(&mutex, NULL) != 0) {
        perror("pthread_mutex_init");
        exit(1);
    }

    if(pthread_mutex_init(&mutex_cond, NULL) != 0) {
        perror("pthread_mutex_init");
        exit(1);
    }

    if(pthread_mutex_init(&mutex_bussy, NULL) != 0) {
        perror("pthread_mutex_init");
        exit(1);
    }

    // init barrier
    printf("Init barrier\n");

    if(pthread_barrier_init(&barrier_renifer, NULL, RENIFER_TRESHOLD) != 0) {
        perror("pthread_barrier_init");
        exit(1);
    }

    if(pthread_barrier_init(&barrier_elf, NULL, ELF_TRESHOLD) != 0) {
        perror("pthread_barrier_init");
        exit(1);
    }

    // init condition
    printf("Init condition\n");

    if(pthread_cond_init(&condition_client, NULL) != 0) {
        perror("pthread_cond_init");
        exit(1);
    }

    if(pthread_cond_init(&condition_santa, NULL) != 0) {
        perror("pthread_cond_init");
        exit(1);
    }

    // ---------------------------------------- //

    // init threads args
    thread_args.runing = &runing;
    thread_args.elf_waiting = &elf_waiting;
    thread_args.renifer_waiting = &renifer_waiting;

    thread_args.mutex = &mutex;
    thread_args.mutex_cond = &mutex_cond;
    thread_args.mutex_bussy = &mutex_bussy;
    thread_args.condition_client = &condition_client;
    thread_args.condition_santa = &condition_santa;

    // init santa args
    santa_args.runing = &runing;
    santa_args.elf_waiting = &elf_waiting;
    santa_args.renifer_waiting = &renifer_waiting;

    santa_args.mutex = &mutex;
    santa_args.mutex_cond = &mutex_cond;
    santa_args.mutex_bussy = &mutex_bussy;
    santa_args.condition_client = &condition_client;
    santa_args.condition_santa = &condition_santa;

    // ---------------------------------------- //

    // Run renifers
    printf("Spawning renifers... (%d)\n", RENIFER_COUNT);
    thread_args.barrier = &barrier_renifer;
    for(i = 0; i < RENIFER_COUNT; i++) {
        pthread_mutex_lock(&mutex);
        printf("Spawn Renifer (%d)\n", i);
        thread_args.id = i;

        if(pthread_create(&thread_tab_renifer[i], NULL, renifer_process, &thread_args) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }

    // Run elfs
    printf("Spawn Elfs... (%d)\n", ELF_COUNT);
    thread_args.barrier = &barrier_elf;
    for(i = 0; i < ELF_COUNT; i++) {
        pthread_mutex_lock(&mutex);
        printf("Spawn Elf (%d)\n", i);
        thread_args.id = i;

        if(pthread_create(&thread_tab_elf[i], NULL, elf_process, &thread_args) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }

    // ---------------------------------------- //

    // Run Santa
    sleep(1);
    santa_process(&santa_args);
    sleep(1);

    // ---------------------------------------- //

    // Wait for threads
    printf("\nWait for threads ...\n");
    for (i = 0; i < RENIFER_COUNT; i++) {
        pthread_join(thread_tab_renifer[i], NULL);
    }
    for (i = 0; i < ELF_COUNT; i++) {
        pthread_join(thread_tab_elf[i], NULL);
    }

    // ---------------------------------------- //

    // cleanup
    printf("Destroy barriers ...\n");
    pthread_barrier_destroy(&barrier_renifer);
    pthread_barrier_destroy(&barrier_elf);

    printf("\nCleanup ...\n");
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&mutex_cond);
    pthread_mutex_destroy(&mutex_bussy);
    pthread_cond_destroy(&condition_client);
    pthread_cond_destroy(&condition_santa);

    return 0;
}
