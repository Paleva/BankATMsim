#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>


typedef sem_t semaphore;

#define SHMKEY 0x1234
#define SEMKEYBANK "/semaphorebank"
#define SEMKEYSERVER "/semaphoreserver"

struct account {
    char nickname[20];
    char password[20];
    int balance;
};

void sendusr1_signal(){
    
}

void exit_handler(int signum, int *cont){
    *cont = 0;
}

int main(){

    struct sigaction sa;
    sa.sa_handler = &exit_handler;
    memset(&sa.sa_mask, 0, sizeof(sa.sa_mask));
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);

    semaphore *sem_bank = sem_open(SEMKEYBANK, 0);
    if(sem_bank == SEM_FAILED){

        printf("%d\n", errno);
        perror("sem_open_bank");
        exit(EXIT_FAILURE);
    }
    semaphore *sem_server = sem_open(SEMKEYSERVER, 0);
    if(sem_bank == SEM_FAILED){
        perror("sem_open_server");
        exit(EXIT_FAILURE);
    }


    int shmid = shmget(SHMKEY, 1024, 0666|IPC_CREAT);
    if (shmid == -1){
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    char *str = (char*) shmat(shmid, NULL, 0);
    if (str == (char*)-1){
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // sendusr1_signal();
    int cont = 1;
    while(cont){
        sem_wait(sem_server);
        printf("Data read from memory: %s\n", str);
        sem_post(sem_bank);
    }

    sem_close(sem_bank);
    sem_close(sem_server);
    sem_destroy(sem_bank);
    sem_destroy(sem_server);
    shmctl(shmid, IPC_RMID, NULL);
    shmdt(str);
    free(str);
    return 0;
}