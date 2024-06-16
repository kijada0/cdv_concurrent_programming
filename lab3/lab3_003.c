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

#define BUFFER_SIZE 5
#define ITERATIONS_COUNT 20

#define SHERED_MEMORY_SIZE (BUFFER_SIZE * sizeof(int))
#define SHERED_MEMORY_NAME "/kk_mem"

pthread_mutex_t mutex;
pthread_cond_t condition_writer, condition_reader;
int global_counter = 0;

// -------------------------------------------------------------------------------- //

typedef struct {
    int *shared_memory_printer;
    int id;
    int intial_value;
} thread_data_t;

// -------------------------------------------------------------------------------- //


int get_random(int min, int max) {
    return min + rand() % (max - min);
}

// -------------------------------------------------------------------------------- //

void *writer_process(void *printer) {
    thread_data_t *data = (thread_data_t *)printer;
    int *shared_memory_printer = data->shared_memory_printer;
    static int i;
    int *address;
    int wait = 0;
    int id = data->id;
    int start = data->intial_value;

    printf("Parent (%d): Start\n", id);

    for(i = 1; i < ITERATIONS_COUNT+1; i++) {
        pthread_mutex_lock(&mutex);

        while(global_counter >= BUFFER_SIZE)
        {   
            pthread_cond_signal(&condition_reader);
            pthread_cond_wait(&condition_writer, &mutex);
        }

        global_counter++;
        // printf("Global counter: %d\n", global_counter);

        address = &shared_memory_printer[i%BUFFER_SIZE];
        *address = i + start;

        printf("Parent (%d): %d\n", id, *address);

        pthread_cond_signal(&condition_reader);
        usleep(get_random(100, 500));

        pthread_mutex_unlock(&mutex);
        usleep(get_random(100, 1000));
    }

    usleep(1000);

    pthread_cond_broadcast(&condition_reader);
    pthread_cond_broadcast(&condition_writer);

    printf("Parent (%d): End\n", id);
}

// -------------------------------------------------------------------------------- //

void *reader_process(void *printer) {
    thread_data_t *data = (thread_data_t *)printer;
    int *shared_memory_printer;
    int i;
    int n = 0;
    int *address;
    int data_ready = 0;
    int id = data->id;
    static int index = 0;

    shared_memory_printer = data->shared_memory_printer;

    printf("Child (%d): Start\n", id);

    for(i = 0; i < ITERATIONS_COUNT; i++) {
        pthread_mutex_lock(&mutex);
        
        while(global_counter <= 0)
        {
            pthread_cond_signal(&condition_writer);
            pthread_cond_wait(&condition_reader, &mutex);
        }

        global_counter--;
        index++;

        address = &shared_memory_printer[index%BUFFER_SIZE];
        n = *address;

        printf("\t\tChild (%d): %d\n", id, n);

        pthread_cond_signal(&condition_writer);
        usleep(get_random(100, 500));

        pthread_mutex_unlock(&mutex);
        usleep(get_random(100, 1000));
    }

    usleep(1000);

    pthread_cond_broadcast(&condition_reader);
    pthread_cond_broadcast(&condition_writer);   

    printf("Child (%d): End\n", id); 
    // TODO: Fix process never ends
}

// -------------------------------------------------------------------------------- //

int main() {
    int shered_memory_id;
    int *shared_memory_printer;
    int i;
    pthread_t thread_tab[4];
    thread_data_t thread_args;

    // init random
    srand(time(NULL));

    printf("Start: %s\n", __FILE__);

    // init shared memory
    printf("Init shared memory\n");
    shered_memory_id = shm_open(SHERED_MEMORY_NAME, O_CREAT | O_RDWR, 0600);
    if(ftruncate(shered_memory_id, SHERED_MEMORY_SIZE) != 0) {
        perror("ftruncate");
        exit(1);
    }
    shared_memory_printer = mmap(NULL, SHERED_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shered_memory_id, 0);

    *shared_memory_printer = 0;

    // init mutex
    printf("Init mutex\n");
    if(pthread_mutex_init(&mutex, NULL) != 0) {
        perror("pthread_mutex_init");
        exit(1);
    }

    // pthread
    printf("\nThread\n");
    thread_args.shared_memory_printer = shared_memory_printer;
    thread_args.intial_value = 0;

    thread_args.id = 1;
    pthread_create(&thread_tab[0], NULL, reader_process, (void *)&thread_args);
    usleep(100);

    thread_args.id = 2;
    pthread_create(&thread_tab[1], NULL, reader_process, (void *)&thread_args);
    usleep(100);

    thread_args.id = 3;
    thread_args.intial_value = 100;
    pthread_create(&thread_tab[3], NULL, writer_process, (void *)&thread_args);
    usleep(100);

    // thread_args.id = 4;
    // thread_args.intial_value = 200;
    // pthread_create(&thread_tab[4], NULL, writer_process, (void *)&thread_args);
    // usleep(100);

    for (i=0; i<3; i++) {
        pthread_join(thread_tab[i], NULL);
    }

    // cleanup
    printf("\nCleanup\n");
    shm_unlink(SHERED_MEMORY_NAME);
    pthread_mutex_destroy(&mutex);

    return 0;
}
