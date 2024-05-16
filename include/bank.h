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
#include <bits/sigaction.h>

typedef sem_t semaphore;

#define SHMKEY 0x1234
#define SEMKEYBANK "/semaphorebank"
#define SEMKEYSERVER "/semaphoreserver"
#define ACCOUNTS "db/accounts.txt"

struct session{
    int current_account;
    pid_t connection_id;
};

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

//
struct session *push_session(struct session *sessions, int current_account, pid_t connection_id, int *session_amount);

// returns 200 if succesful
int fetch_balance(int current_account);

// returns 202 if successfully withdrawn 
// returns 400 if illegal withdraw 
int withdraw_money(struct account *accounts, char buffer[], int current_account);

// returns 201 if account is created
int create_acc(struct account *accounts, char buffer[], int *accounts_amount);

// returns 202 if succesfully deposited
int deposit_money(struct account *accounts, char buffer[], int current_account);

// returns 200 if account is found
// returns 404 if account is not found
int login(struct account *accounts, char path[], int accounts_amount);

// Return the index of the accounts in the array if it exist
// Otherwise return -1
int check_if_acc_exists(struct account *accounts, char nickname[], int accounts_amount);

// reads accounts from the file
struct account *read_accounts(int *account_number);

// pushes an entry of an account to the array
struct account *push_account(struct account *accounts, char nickname[], char password[], int balance, int *account_number);

// updates the file with the accounts
void update_db(struct account *accounts, int *account_number);

semaphore *semaphore_open(char* name, int init_val);

int shared_mem_id(int key);
char *shared_mem_ptr(int key);

void sigusr1_handler(int signum, siginfo_t *info, void *ucontext);
void sigchld_handler(int signum);
void sigint_handler(int signum);

void sigusr1_send(int pid);

void send_to_server(struct bank_server *bank_server);
void read_from_server(struct bank_server *bank_server);

#endif // BANK_H