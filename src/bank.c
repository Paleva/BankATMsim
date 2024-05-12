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

volatile int data_ready = 0;
volatile int exit_flag = 0;

struct account {
    char nickname[20];
    char password[20];
    int balance;
};

struct bank_server{
    semaphore *sem_bank;
    semaphore *sem_server;
    char *shared_mem;
    pid_t server_pid;
    char buffer[1024];
};


int create_acc();

int check_if_acc_exists(struct account *accounts, char nickname[], int accounts_amount);

void send_password();
void verify_password();
void deposit_money();

semaphore *semaphore_open(char* name, int init_val);

struct account *read_accounts(char *filename, int *account_number);

int shared_mem_id(int key);
char *shared_mem_ptr(int key);

void sigusr1_handler(int signum, siginfo_t *info, void *ucontext);
void sigchld_handler(int signum);
void sigint_handler(int signum);

void sigusr1_send(int pid);

void send_to_server(struct bank_server *bank_server);
void read_from_server(struct bank_server *bank_server);

struct bank_server *bank_server;

int main(){
    int account_amount = 0;
    struct account *accounts = read_accounts(ACCOUNTS, &account_amount);
    bank_server = malloc(sizeof(struct bank_server));

    sem_unlink(SEMKEYBANK);
    sem_unlink(SEMKEYSERVER);
    int shmid = shared_mem_id(SHMKEY);
    bank_server->shared_mem = shared_mem_ptr(shmid);
    bank_server->sem_bank = semaphore_open(SEMKEYBANK, 0);
    bank_server->sem_server = semaphore_open(SEMKEYSERVER, 1);
    bank_server->server_pid = 0;

    struct sigaction sa;
    bzero(&sa, sizeof(sa));
    sa.sa_sigaction = &sigusr1_handler;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
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

    struct sigaction sa_sigint;
    bzero(&sa, sizeof(sa_sigint));
    sa_sigint.sa_handler = &sigint_handler;    
    if(sigaction(SIGINT, &sa_sigint, NULL) == -1){
        perror("sigaction");
        exit(EXIT_FAILURE);
    }


    pid_t server_proc = fork();
    if(server_proc < 0){
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if(server_proc == 0){
        execl("./server", "./server", (char*)NULL);
        printf("Server pid: %d\n", server_proc);
    }
    else {
        while (1) {
            if (data_ready == 0) {
                continue;
            } 
            else {
                printf("Data ready!!!!1\n");
                read_from_server(bank_server);
                int index = check_if_acc_exists(accounts, bank_server->buffer, account_amount);
                if (index > 0) {
                    strcpy(bank_server->buffer, accounts[index].password);
                    send_to_server(bank_server);
                    data_ready = 0;
                } else {
                    strcpy(bank_server->buffer, "NOACC");
                    send_to_server(bank_server);
                    data_ready = 0;
                }
            }
            if(exit_flag == 1){
                sem_close(bank_server->sem_bank);
                sem_close(bank_server->sem_server);
                sem_destroy(bank_server->sem_bank);
                sem_destroy(bank_server->sem_server);
                shmctl(shmid, IPC_RMID, NULL);
                shmdt(bank_server->shared_mem);
                free(bank_server);
                free(accounts);
                exit(EXIT_SUCCESS);
            }
        }
    }

    return 0;
}

void send_to_server(struct bank_server *bank_server){
    sem_wait(bank_server->sem_bank);
    strcpy(bank_server->shared_mem, bank_server->buffer);
    sem_post(bank_server->sem_server);
    sem_post(bank_server->sem_server);
    sigusr1_send(bank_server->server_pid);
}

void read_from_server(struct bank_server *bank_server){
    sem_wait(bank_server->sem_bank);
    strcpy(bank_server->buffer, bank_server->shared_mem);
    sem_post(bank_server->sem_server);
}

semaphore *semaphore_open(char *name, int init_val){
    semaphore *sem = sem_open(name, O_CREAT, 0644, init_val);
    if(sem == SEM_FAILED){
        perror("sem_open_bank");
        exit(EXIT_FAILURE);
    }
    return sem;
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

void sigusr1_send(int pid){
    kill(pid, SIGUSR1);
}

void sigusr1_handler(int signum, siginfo_t *info, void *ucontext){
    data_ready = 1;
    bank_server->server_pid = info->si_pid;
}


void sigchld_handler(int signum){
    while(waitpid(-1, NULL, WNOHANG) > 0); // figure how this works
}

void sigint_handler(int signum){
    exit_flag = 1;
    printf("Exiting\n");
}