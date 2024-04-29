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
#include <bits/sigaction.h>

typedef sem_t semaphore;

#define SHMKEY 0x1234
#define SEMKEYBANK "/semaphorebank"
#define SEMKEYSERVER "/semaphoreserver"
#define ACCOUNTS "db/accounts.txt"

// IMPLEMENT SIGNALS BETWEEN SERVER AND BANK
// FOR NOTIFYING IF THE SERVER IS REQUESTING INFORMATION

// return 1 to server on failure and 0 on success in creation of account
int create_acc();

// return 1 to server on failure and 0 on success in login
int check_if_acc_exists();

void send_password();
void verify_password();
void deposit_money();

semaphore *semaphore_open(char* name, int init_val);
struct account *read_accounts(char *filename, int *account_number);
int shared_mem_id(int key);
char *shared_mem_ptr(int key);

void sigusr1_handler(int signum);
void sigusr1_send(int pid);

void do_with_semaphores(char* shared, char *str, sem_t *semaphore, sem_t *sem_server);

struct account {
    char nickname[20];
    char password[20];
    int balance;
};

int main(){
    int shmid = shared_mem_id(SHMKEY);
    char *shmptr = shared_mem_ptr(shmid);
    int account_amount = 0;
    struct account *accounts = read_accounts(ACCOUNTS, &account_amount);
    sem_unlink(SEMKEYBANK);
    sem_unlink(SEMKEYSERVER);
    semaphore *sem_bank = semaphore_open(SEMKEYBANK, 1);
    semaphore *sem_server = semaphore_open(SEMKEYSERVER, 0);

    struct sigaction sa;
    sa.sa_handler = &sigusr1_handler;
    sigaction(SIGUSR1, &sa, NULL);



    pid_t server_proc = fork();
    if(server_proc < 0){
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if(server_proc == 0){
        execl("./server", NULL);
        printf("Server pid: %d\n", server_proc);
    }
    printf("Server pid: %d\n", server_proc);

    sleep(1);
    sigusr1_send(server_proc);
    do_with_semaphores(shmptr, "Hello from bank", sem_bank, sem_server);
    sem_close(sem_bank);
    sem_close(sem_server);
    sem_destroy(sem_bank);
    sem_destroy(sem_server);
    shmctl(shmid, IPC_RMID, NULL);
    shmdt(shmptr);
    free(accounts);
    return 0;
}

void do_with_semaphores(char* shared, char *str, semaphore *sem_bank, semaphore *sem_server){

    sem_wait(sem_bank);
    printf("Data written in memory: %s\n", str);
    strcpy(shared, str);
    sem_post(sem_server);

}

semaphore *semaphore_open(char *name, int init_val){
    semaphore *sem = sem_open(name, O_CREAT, 0644, init_val);
    if(sem == SEM_FAILED){
        perror("sem_open_bank");
        exit(EXIT_FAILURE);
    }
    return sem;
}

struct account *read_accounts(char *filename, int *account_number){
    FILE *file;
    file = fopen(filename, "r");
    if(file == NULL){
        perror("Opening file");
        exit(EXIT_FAILURE);
    }

    char line[1024];
    struct account *accounts = NULL;
    //read "database" into memory
    while(fgets(line, 1024, file) != NULL){
        char *nickname = strtok(line, ":");
        char *password = strtok(NULL, ":");
        char *balance = strtok(NULL, ":");
        accounts = realloc(accounts, (*account_number + 1) * sizeof(struct account));
        if(accounts == NULL){
            perror("Memory allocation");
            exit(EXIT_FAILURE);
        }
        strcpy(accounts[*account_number].nickname, nickname);
        strcpy(accounts[*account_number].password, password);
        accounts[*account_number].balance = atoi(balance);
        (*account_number)++;
    }
    fclose(file);
    return accounts;
}

int shared_mem_id(int key){
    int shmid = shmget(key, 1024, 0666|IPC_CREAT);
    if (shmid == -1){
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    return shmid;
}

char *shared_mem_ptr(int shmid){    
    char *str = (char*) shmat(shmid, NULL, 0);
    if (str == (char*)-1){
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    return str;
}


void sigusr1_handler(int signum){
    printf("Received signal %d\n", signum);
}

void sigusr1_send(int pid){
    kill(pid, SIGUSR1);
}
