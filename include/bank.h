#ifndef BANK_H
#define BANK_H

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


struct account {
    char nickname[20];
    char password[20];
    int64_t balance;
};

struct bank_server{
    semaphore *sem_bank;
    semaphore *sem_server;
    char *shared_mem;
    pid_t server_pid;
    char buffer[1024];
};

//201 CREATED
int create_acc(struct account *accounts, char buffer[], int *accounts_amount);
//202 ACCEPTED
int deposit_money();
// 200 OK or 404 NOTFOUND
int login(struct account *accounts, char path[], int accounts_amount);
int check_if_acc_exists(struct account *accounts, char nickname[], int accounts_amount);

semaphore *semaphore_open(char* name, int init_val);

struct account *read_accounts(int *account_number);
struct account *push(struct account *accounts, char nickname[], char password[], int balance, int *account_number);
void update_db(struct account *accounts, int *account_number);

int shared_mem_id(int key);
char *shared_mem_ptr(int key);

void sigusr1_handler(int signum, siginfo_t *info, void *ucontext);
void sigchld_handler(int signum);
void sigint_handler(int signum);

void sigusr1_send(int pid);

void send_to_server(struct bank_server *bank_server);
void read_from_server(struct bank_server *bank_server);

#endif // BANK_H