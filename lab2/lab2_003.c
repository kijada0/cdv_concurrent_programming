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

void *thread_func(void *printer){    
    auto int auto_int = 0;
    register int register_int = 0;
    static int static_int = 0;
    static __thread int static_thread_int = 0;

    printf("Thread: Start %d\n", printer);

    printf("Thread: auto_int: %d\n", auto_int);
    printf("Thread: register_int: %d\n", register_int);
    printf("Thread: static_int: %d\n", static_int);
    printf("Thread: static_thread_int: %d\n", static_thread_int);
}

// -------------------------------------------------------------------------------- //

int main() {
    pthread_t thread_A, thread_B;
    int i;

    printf("Start: %s\n", __FILE__);

    pthread_create(&thread_A, NULL, thread_func, (void *)1);
    pthread_create(&thread_B, NULL, thread_func, (void *)2);

    pthread_join(thread_A, NULL);
    pthread_join(thread_B, NULL);

    return 0;
}
