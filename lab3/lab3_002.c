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

int get_random(int min, int max) {
    return min + rand() % (max - min);
}

// -------------------------------------------------------------------------------- //

void *writer_process(void *printer) {
    int *shared_memory_printer = (int *)printer;
    int i, j = 0;
    int *address;
    int wait = 0;

    printf("Parent: Start\n");

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
        *address = i;

        printf("Parent: %d\n", i);

        pthread_cond_signal(&condition_reader);
        usleep(get_random(100, 500));

        pthread_mutex_unlock(&mutex);
        usleep(get_random(100, 1000));
    }
}

// -------------------------------------------------------------------------------- //

void *reader_process(void *printer) {
    int *shared_memory_printer = (int *)printer;
    int i;
    int n = 0;
    int *address;
    int data_ready = 0;

    printf("Child: Start\n");

    for(i = 1; i < ITERATIONS_COUNT+1; i++) {
        pthread_mutex_lock(&mutex);
        
        while(global_counter <= 0)
        {
            pthread_cond_signal(&condition_writer);
            pthread_cond_wait(&condition_reader, &mutex);
        }

        global_counter--;
        // printf("\t\tGlobal counter: %d\n", global_counter);

        address = &shared_memory_printer[i%BUFFER_SIZE];
        n = *address;

        printf("\t\tChild: %d\n", n);

        pthread_cond_signal(&condition_writer);
        usleep(get_random(100, 500));

        pthread_mutex_unlock(&mutex);
        usleep(get_random(100, 1000));
    }

}

// -------------------------------------------------------------------------------- //

int main() {
    int shered_memory_id;
    int *shared_memory_printer;
    int pid;
    pthread_t thread_parent, thread_child;

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

    pthread_create(&thread_child, NULL, reader_process, (void *)shared_memory_printer);
    pthread_create(&thread_parent, NULL, writer_process, (void *)shared_memory_printer);

    pthread_join(thread_child, NULL);
    pthread_join(thread_parent, NULL);

    // cleanup
    printf("\nCleanup\n");
    shm_unlink(SHERED_MEMORY_NAME);
    pthread_mutex_destroy(&mutex);

    return 0;
}
