#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
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

int data_ready = 0;

struct account {
    char nickname[20];
    char password[20];
    int balance;
};

struct bank_to_server{
    int server_pid;
    char *shared_mem;
    char *buffer;
    semaphore *sem_bank;
    semaphore *sem_server;
};

// return 1 to server on failure and 0 on success in creation of account
int create_acc();

// return 1 to server on failure and 0 on success in login
int check_if_acc_exists(struct account *accounts, char nickname[], int accounts_amount);

void send_password();
void verify_password();
void deposit_money();

semaphore *semaphore_open(char* name, int init_val);
struct account *read_accounts(char *filename, int *account_number);
int shared_mem_id(int key);
char *shared_mem_ptr(int key);

void sigchld_handler(int signo);
void sigusr1_handler(int signum);
void sigusr1_send(int pid);

void send_to_server(struct bank_to_server *server_bank);
void read_from_server(struct bank_to_server *server_bank);


int main(){
    int account_amount = 0;
    struct account *accounts = read_accounts(ACCOUNTS, &account_amount);
    struct bank_to_server *server_bank = malloc(sizeof(struct bank_to_server));

    sem_unlink(SEMKEYBANK);
    sem_unlink(SEMKEYSERVER);
    int shmid = shared_mem_id(SHMKEY);
    server_bank->shared_mem = shared_mem_ptr(shmid);
    server_bank->sem_bank = semaphore_open(SEMKEYBANK, 0);
    server_bank->sem_server = semaphore_open(SEMKEYSERVER, 1);

    struct sigaction sa;
    bzero(&sa, sizeof(sa));
    sa.sa_handler = &sigusr1_handler;
    sa.sa_flags = SA_RESTART;
    if(sigaction(SIGUSR1, &sa, NULL) == -1){
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa_chld;
    bzero(&sa, sizeof(sa_chld));
    sa_chld.sa_handler = &sigchld_handler;
    sa_chld.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &sa_chld, NULL) == -1){
        perror("sigaction");
        exit(EXIT_FAILURE);
    }


    server_bank->server_pid = fork();
    if (server_bank->server_pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (server_bank->server_pid == 0) {
        execl("./server", "./server");
    } 
    else {
        while (1) {
            if (data_ready == 0) {
                continue;
            } 
            else {
                printf("Data ready!!!!1\n");
                read_from_server(server_bank);
                int index = check_if_acc_exists(accounts, server_bank->buffer, account_amount);
                if (index > 0) {
                    server_bank->buffer = malloc(sizeof(accounts[index].password));
                    strcpy(server_bank->buffer, accounts[index].password);
                    send_to_server(server_bank);
                    free(server_bank->buffer);
                    data_ready = 0;
                } else {
                    strcpy(server_bank->buffer, "NOACC");
                    send_to_server(server_bank);
                    free(server_bank->buffer);
                    data_ready = 0;
                }
            }
        }
    }

    sem_destroy(server_bank->sem_bank);
    sem_destroy(server_bank->sem_server);
    sem_close(server_bank->sem_bank);
    sem_close(server_bank->sem_server);
    shmctl(shmid, IPC_RMID, NULL);
    shmdt(server_bank->shared_mem);
    free(accounts);
    return 0;
}

void send_to_server(struct bank_to_server *server_bank){
    sem_wait(server_bank->sem_bank);
    sigusr1_send(server_bank->server_pid);
    printf("BANK: Data written in memory: %s\n", server_bank->buffer);
    strcpy(server_bank->shared_mem, server_bank->buffer);
    bzero(server_bank->buffer, 1024);
    sem_post(server_bank->sem_server);
}

void read_from_server(struct bank_to_server *server_bank){
    sem_wait(server_bank->sem_bank);
    server_bank->buffer = malloc(1024);
    printf("BANK: Data read from memory: %s\n", server_bank->shared_mem);
    strcpy(server_bank->buffer, server_bank->shared_mem);
    bzero(server_bank->shared_mem, 1024);
    sem_post(server_bank->sem_server);
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

int check_if_acc_exists(struct account *accounts, char nickname[], int accounts_amount){
    int i;
    printf("BANK IS CHECKING \n");
    for (i = 0; i < accounts_amount; i++){
        printf("BANK: %s\n", accounts[i].nickname);
        if(strcmp(accounts[i].nickname, nickname) == 0){
            printf("ACC FOUND");
            return i;
        }
    }
    return -1;
}

semaphore *semaphore_open(char *name, int init_val){
    semaphore *sem = sem_open(name, O_CREAT, 0644, init_val);
    if(sem == SEM_FAILED){
        perror("sem_open_bank");
        exit(EXIT_FAILURE);
    }
    return sem;
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
    // printf("GOTTEN:BANK\n");
    data_ready = 1;
}

void sigusr1_send(int pid){
    // printf("SENT:BANK\n");
    kill(pid, SIGUSR1);
}

int create_acc();

void sigchld_handler(int signo){
    while(waitpid(-1, NULL, WNOHANG) > 0); // figure how this works
}