#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

int main() {
    int d;
    int *prt;
    int pid;

    printf("Start \n");

    // Create shared memory
    d = shm_open("/kk_mem", O_CREAT | O_RDWR, 0600);
    ftruncate(d, 1024);
    prt = mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_SHARED, d, 0);
    *prt = 0;

    pid = fork();

    if(pid == 0) {
        sleep(1);
        *prt = getpid();
    }
    else {
        while(*prt == 0) {
            printf(".");
            sleep(1);
        }
    }

    printf("End \t pid = %d \t prt = %d \n", pid, *prt);

    sleep(2);
    shm_unlink("/kk_mem");

    return 0;
}
