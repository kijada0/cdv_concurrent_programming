#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include "barrier.h"

// -------------------------------------------------------------------------------- //

#define RENIFER_COUNT       9
#define RENIFER_TRESHOLD    9

#define ELF_COUNT           10
#define ELF_TRESHOLD        3

#define ITERATIONS_COUNT    10

#define NEED_HELP_PROB_R    5
#define NEED_HELP_PROB_E    8

// -------------------------------------------------------------------------------- //

typedef struct {
    int id;
    char name[32];
    char message_prefix[32];
    void (*task_func)(uint8_t, char *);
    uint8_t *runing;
    uint8_t *waiting_queue;
    uint8_t *state;
    uint8_t help_probability;
    pthread_mutex_t *mutex;
    pthread_mutex_t *mutex_bussy;
    pthread_mutex_t *mutex_bussy_santa;
    pthread_cond_t *condition_client;
    pthread_cond_t *condition_santa;
    pthread_cond_t *condition_finnish;
    barrier_t *barrier;
} thread_data_t;

typedef struct {
    uint8_t *runing;
    uint8_t *elf_waiting;
    uint8_t *renifer_waiting;
    pthread_mutex_t *mutex;
    pthread_mutex_t *mutex_bussy_santa;
    pthread_cond_t *condition_renifer;
    pthread_cond_t *condition_elf;
    pthread_cond_t *condition_santa;
    pthread_cond_t *condition_finnish;
} santa_data_t;

typedef struct {
    uint8_t *runing;
    uint8_t *renifer_waiting;
    uint8_t *renifer_state_tab;
    uint8_t *elf_waiting;
    uint8_t *elf_state_tab;
    pthread_mutex_t *mutex;
} santa_wife_data_t;

// -------------------------------------------------------------------------------- //

int get_random(int min, int max) {
    return min + rand() % (max - min);
}

// -------------------------------------------------------------------------------- //

void santa_process(santa_data_t *params) {
    printf("\n--------------------------------------------------------------------------------\n");
    printf("Santa: Start\n");
    uint8_t completed_iterations = 0;
    uint8_t i, r_count = 0, e_count = 0;

    while(1) {
        // wait for signal
        printf("Santa: Waiting for signal\n");
        pthread_mutex_lock(params->mutex_bussy_santa);
        pthread_cond_wait(params->condition_santa, params->mutex_bussy_santa);
        printf("Santa: Wake up\n");
        sleep(1);
        
        // check who wake up
        pthread_mutex_lock(params->mutex);
        r_count = *params->renifer_waiting;
        e_count = *params->elf_waiting;
        printf("Santa: Renifers: %d, Elfs: %d\n", r_count, e_count);
        pthread_mutex_unlock(params->mutex);

        pthread_mutex_lock(params->mutex);
        if(*params->renifer_waiting >= RENIFER_TRESHOLD) {
            pthread_mutex_unlock(params->mutex);

            printf("Santa: Help Renifers\n");
            sleep(1);
            pthread_cond_broadcast(params->condition_renifer);
            printf("Santa: Broadcast Renifers\n");

            pthread_mutex_lock(params->mutex);
            r_count = *params->renifer_waiting;
            for (i = 0; i < r_count; i++) {
                printf("Santa: Remaing Renifers: %d\n", r_count - i);
                pthread_cond_wait(params->condition_finnish, params->mutex);
                printf("Santa: Help Renifers: %d\n", i);
            }
            pthread_mutex_unlock(params->mutex);

            printf("Santa: End Help Renifers\n");
            completed_iterations++;
        }
        pthread_mutex_unlock(params->mutex);

        pthread_mutex_lock(params->mutex);
        if(*params->elf_waiting >= ELF_TRESHOLD) {
            pthread_mutex_unlock(params->mutex);

            printf("Santa: Help Elfs\n");
            sleep(1);
            pthread_cond_broadcast(params->condition_elf);
            printf("Santa: Broadcast Elfs\n");

            pthread_mutex_lock(params->mutex);
            e_count = *params->elf_waiting;
            for (i = 0; i < e_count; i++) {
                printf("Santa: Remaing Elfs: %d\n", e_count - i);
                pthread_cond_wait(params->condition_finnish, params->mutex);
                printf("Santa: Help Elfs: %d\n", i);
            }
            pthread_mutex_unlock(params->mutex);

            printf("Santa: End Help Elfs\n");
            completed_iterations++;
        }
        pthread_mutex_unlock(params->mutex);

        printf("->\tSanta: Completed iterations: <%d>\n", completed_iterations);
        if(completed_iterations >= ITERATIONS_COUNT) {
            break;
        }

        pthread_mutex_unlock(params->mutex_bussy_santa);
    }

    pthread_mutex_lock(params->mutex);
    *params->runing = 0;
    sleep(1);
    pthread_mutex_unlock(params->mutex);

    pthread_mutex_unlock(params->mutex_bussy_santa);

    printf("Santa: End\n");
}

// -------------------------------------------------------------------------------- //

void *worker_process(void *args) {
    thread_data_t params;
    pthread_mutex_t *mutex;
    int r;
    void (*task)(uint8_t, char *);

    memcpy(&params, (thread_data_t *)args, sizeof(thread_data_t));
    mutex = params.mutex;
    *params.state = 0;
    pthread_mutex_unlock(mutex);

    sleep(1);
    printf("%s%s (%d): Start\n", params.message_prefix, params.name, params.id);

    while(1) {
        pthread_mutex_lock(params.mutex_bussy_santa);
        pthread_mutex_unlock(params.mutex_bussy_santa);

        pthread_mutex_lock(mutex);
        printf("%s%s (%d) runing: %d\n", params.message_prefix, params.name, params.id, *params.runing);
        if(*params.runing == 0) {
            *params.state = 9;
            pthread_mutex_unlock(mutex);
            break;
        }
        pthread_mutex_unlock(mutex);

        r = rand() % params.help_probability;
        if(r == 0) {
            printf("%s%s (%d): Waiting for Santa\n", params.message_prefix, params.name, params.id);
            pthread_mutex_lock(mutex);
            *params.waiting_queue += 1;
            *params.state = 1;
            pthread_mutex_unlock(mutex);
            
            barrier_wait(params.barrier);
            printf("%s%s (%d): Barrier passed\n", params.message_prefix, params.name, params.id);

            // Check if Santa is runing
            pthread_mutex_lock(mutex);
            if(*params.runing == 0) {
                *params.state = 9;
                pthread_mutex_unlock(mutex);
                break;
            }
            *params.state = 2;
            pthread_mutex_unlock(mutex);
            
            pthread_mutex_lock(params.mutex_bussy);
            printf("%s%s (%d): Wake up Santa...\n", params.message_prefix, params.name, params.id);
            pthread_cond_broadcast(params.condition_santa);

            pthread_cond_wait(params.condition_client, params.mutex_bussy);
            printf("%s%s (%d): Runing with Santa...\n", params.message_prefix, params.name, params.id);

            // Run task
            pthread_mutex_lock(mutex);
            *params.state = 3;
            task = params.task_func;
            task(params.id, params.message_prefix);
            pthread_mutex_unlock(mutex);
            
            sleep(1);
            printf("%s%s (%d): Finish send finnish signal\n", params.message_prefix, params.name, params.id);
            pthread_cond_signal(params.condition_finnish);

            pthread_mutex_lock(params.mutex);
            *params.waiting_queue -= 1;
            *params.state = 0;
            printf("%s%s (%d): End Runing with Santa\n", params.message_prefix, params.name, params.id);
            pthread_mutex_unlock(params.mutex);

            pthread_mutex_unlock(params.mutex_bussy);
        }

        usleep(get_random(100, 2500) * 1000);
    }

    printf("%s%s (%d): End\n", params.message_prefix, params.name, params.id);
}

// -------------------------------------------------------------------------------- //

void renifer_task(uint8_t id, char *prefix) {
    printf("%s\tRenifer (%d): Start Working\n", prefix, id);
    usleep(get_random(100, 2500) * 1000); // simulate work
    printf("%s\tRenifer (%d): End Working\n", prefix, id);
}

void elf_task(uint8_t id, char *prefix) {
    printf("%s\tElf (%d): Start Working\n", prefix, id);
    usleep(get_random(100, 2500) * 1000); // simulate work
    printf("%s\tElf (%d): End Working\n", prefix, id);
}

// -------------------------------------------------------------------------------- //

void *santa_wife_process(void *args) {
    santa_wife_data_t *params = (santa_wife_data_t *)args;
    uint8_t i;
    char elf_status_str[ELF_COUNT + 1];
    char renifer_status_str[RENIFER_COUNT + 1];

    printf("Santa's wife: Start\n");

    while(1) {
        pthread_mutex_lock(params->mutex);
        if(*params->runing == 0) {
            pthread_mutex_unlock(params->mutex);
            break;
        }
        pthread_mutex_unlock(params->mutex);

        pthread_mutex_lock(params->mutex);


        for(i = 0; i < RENIFER_COUNT; i++) {
            switch(params->renifer_state_tab[i]) {
                case 0:
                    renifer_status_str[i] = '~';
                    break;
                case 1:
                    renifer_status_str[i] = 'W';
                    break;
                case 2:
                    renifer_status_str[i] = 'B';
                    break;
                case 3:
                    renifer_status_str[i] = 'R';
                    break;
                case 9:
                    renifer_status_str[i] = 'X';
                    break;
            }
        }
        renifer_status_str[RENIFER_COUNT] = '\0';

        for(i = 0; i < ELF_COUNT; i++) {
            switch(params->elf_state_tab[i]) {
                case 0:
                    elf_status_str[i] = '~';
                    break;
                case 1:
                    elf_status_str[i] = 'W';
                    break;
                case 2:
                    elf_status_str[i] = 'B';
                    break;
                case 3:
                    elf_status_str[i] = 'R';
                    break;
                case 9:
                    elf_status_str[i] = 'X';
                    break;
            }
        }
        elf_status_str[ELF_COUNT] = '\0';
        printf("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t[WIFE] \t renifer: waiting: %d | %s \t elf: waiting: %d | %s\n", *params->renifer_waiting, renifer_status_str, *params->elf_waiting, elf_status_str);
        
        pthread_mutex_unlock(params->mutex);

        sleep(1);
    }

    pthread_mutex_lock(params->mutex);
    *params->runing = 0;
    pthread_mutex_unlock(params->mutex);

    printf("Santa's wife: End\n");
}


// -------------------------------------------------------------------------------- //

int main() {
    int i;
    uint8_t runing = 1;
    uint8_t elf_waiting = 0, renifer_waiting = 0;
    uint8_t elf_state[ELF_COUNT] = {0}, renifer_state[RENIFER_COUNT] = {0};

    santa_data_t santa_args;
    thread_data_t thread_args;
    pthread_t thread_tab_renifer[RENIFER_COUNT];
    pthread_t thread_tab_elf[ELF_COUNT];
    pthread_t thread_santa_wife;

    pthread_mutex_t mutex;
    pthread_mutex_t mutex_cond;
    pthread_mutex_t mutex_bussy;
    pthread_mutex_t mutex_bussy_santa;

    pthread_cond_t condition_renifer;
    pthread_cond_t condition_elf;
    pthread_cond_t condition_santa;
    pthread_cond_t condition_finnish;

    barrier_t barrier_renifer;
    barrier_t barrier_elf;
    santa_wife_data_t santa_wife_args;

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

    if(pthread_mutex_init(&mutex_bussy, NULL) != 0) {
        perror("pthread_mutex_init");
        exit(1);
    }

    if(pthread_mutex_init(&mutex_bussy_santa, NULL) != 0) {
        perror("pthread_mutex_init");
        exit(1);
    }

    // init barrier
    printf("Init barrier\n");

    if(barrier_init(&barrier_renifer, RENIFER_TRESHOLD) != 0) {
        perror("pthread_barrier_init");
        exit(1);
    }

    if(barrier_init(&barrier_elf, ELF_TRESHOLD) != 0) {
        perror("pthread_barrier_init");
        exit(1);
    }

    // init condition
    printf("Init condition\n");

    if(pthread_cond_init(&condition_renifer, NULL) != 0) {
        perror("pthread_cond_init");
        exit(1);
    }

    if(pthread_cond_init(&condition_elf, NULL) != 0) {
        perror("pthread_cond_init");
        exit(1);
    }

    if(pthread_cond_init(&condition_santa, NULL) != 0) {
        perror("pthread_cond_init");
        exit(1);
    }

    if(pthread_cond_init(&condition_finnish, NULL) != 0) {
        perror("pthread_cond_init");
        exit(1);
    }

    // ---------------------------------------- //

    // init threads args
    thread_args.runing = &runing;

    thread_args.mutex = &mutex;
    thread_args.mutex_bussy = &mutex_bussy;
    thread_args.mutex_bussy_santa = &mutex_bussy_santa;

    thread_args.condition_santa = &condition_santa;
    thread_args.condition_finnish = &condition_finnish;

    // init santa args
    santa_args.runing = &runing;
    santa_args.elf_waiting = &elf_waiting;
    santa_args.renifer_waiting = &renifer_waiting;

    santa_args.mutex = &mutex;
    santa_args.mutex_bussy_santa = &mutex_bussy_santa;

    santa_args.condition_renifer = &condition_renifer;
    santa_args.condition_elf = &condition_elf;
    santa_args.condition_santa = &condition_santa;
    santa_args.condition_finnish = &condition_finnish;

    // init santa wife args
    santa_wife_args.runing = &runing;
    santa_wife_args.elf_waiting = &elf_waiting;
    santa_wife_args.renifer_waiting = &renifer_waiting;
    santa_wife_args.elf_state_tab = elf_state;
    santa_wife_args.renifer_state_tab = renifer_state;

    santa_wife_args.mutex = &mutex;

    // ---------------------------------------- //

    // Run renifers
    printf("Spawning renifers... (%d)\n", RENIFER_COUNT);
    strcpy(thread_args.name, "Renifer");
    strcpy(thread_args.message_prefix, "\t\t\t\t\t");
    thread_args.waiting_queue = &renifer_waiting;
    thread_args.help_probability = NEED_HELP_PROB_R;
    thread_args.task_func = &renifer_task;
    thread_args.barrier = &barrier_renifer;
    thread_args.condition_client = &condition_renifer;

    for(i = 0; i < RENIFER_COUNT; i++) {
        pthread_mutex_lock(&mutex);
        printf("Spawn Renifer (%d)\n", i);
        thread_args.id = i;
        thread_args.state = &renifer_state[i];

        if(pthread_create(&thread_tab_renifer[i], NULL, worker_process, &thread_args) != 0) {
            perror("pthread_create");
            exit(1);
        }
        usleep(1000);
    }

    // Run elfs
    printf("Spawn Elfs... (%d)\n", ELF_COUNT);
    strcpy(thread_args.name, "Elf");
    strcpy(thread_args.message_prefix, "\t\t\t\t\t\t\t\t\t\t");
    thread_args.waiting_queue = &elf_waiting;
    thread_args.help_probability = NEED_HELP_PROB_E;
    thread_args.task_func = &elf_task;
    thread_args.barrier = &barrier_elf;
    thread_args.condition_client = &condition_elf;

    for(i = 0; i < ELF_COUNT; i++) {
        pthread_mutex_lock(&mutex);
        printf("Spawn Elf (%d)\n", i);
        thread_args.id = i;
        thread_args.state = &elf_state[i];

        if(pthread_create(&thread_tab_elf[i], NULL, worker_process, &thread_args) != 0) {
            perror("pthread_create");
            exit(1);
        }
        usleep(1000);
    }

    // ---------------------------------------- //

    // Run Santa's wife
    printf("Spawning Santa's wife...\n");
    if(pthread_create(&thread_santa_wife, NULL, santa_wife_process, &santa_wife_args) != 0) {
        perror("pthread_create");
        exit(1);
    }

    // ---------------------------------------- //

    // Run Santa
    santa_process(&santa_args);

    printf("\nRelease barriers ...\n");
    barrier_release(&barrier_renifer);
    barrier_release(&barrier_elf);

    // ---------------------------------------- //

    // Wait for threads
    printf("\nWait for threads ...\n");
    sleep(2);

    for (i = 0; i < RENIFER_COUNT; i++) {
        pthread_join(thread_tab_renifer[i], NULL);
        printf("\tRenifer (%d) joined\n", i);
    }

    for (i = 0; i < ELF_COUNT; i++) {
        pthread_join(thread_tab_elf[i], NULL);
        printf("\tElf (%d) joined\n", i);
    }

    pthread_join(thread_santa_wife, NULL);

    // ---------------------------------------- //

    // destroy barriers
    printf("Destroy barriers ...\n");
    barrier_destroy(&barrier_renifer);
    barrier_destroy(&barrier_elf);

    // cleanup
    printf("\nCleanup ...\n");
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&mutex_bussy_santa);

    pthread_cond_destroy(&condition_renifer);
    pthread_cond_destroy(&condition_elf);
    pthread_cond_destroy(&condition_santa);

    return 0;
}
