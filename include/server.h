#ifndef SERVER_H
#define SERVER_H


#include <unistd.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <signal.h>

typedef sem_t semaphore;

#define SHMKEY 0x1234
#define PORT 8080
#define MAX_CONNECTIONS 5
#define SEMKEYBANK "/semaphorebank"
#define SEMKEYSERVER "/semaphoreserver"



struct server_bank{
    int bank_pid;
    char *shared_mem;
    char buffer[1024];
    semaphore *sem_bank;
    semaphore *sem_server;
};

semaphore *semaphore_open(char* name);

int shared_mem_id(int key);
char *shared_mem_ptr(int key);

void read_from_bank(struct server_bank *server_bank);
void send_to_bank(struct server_bank *server_bank);

void deposit_money(int socket, char buffer[], struct server_bank *server_bank); 
void fetch_balance(int socket, char buffer[], struct server_bank *server_bank);
void withdraw_money(int socket, char buffer[], struct server_bank *server_bank);
void exit_server(int socket, struct server_bank *server_bank);

void handle_login(int socket, char buffer[], struct server_bank *server_bank);
void handle_create_acc(int socket, char buffer[], struct server_bank *server_bank);

int handle_request(int socket, struct server_bank *server_bank);
void handle_get_request(int socket, char buffer[], struct server_bank *server_bank);
void handle_put_request(int socket, char buffer[], struct server_bank *server_bank);
void send_response(int sock, char *request);

void sigsusr1_send(int pid);
void sigchld_handler(int signum);
void sigusr1_handler(int signum);
void sigint_handler(int signum);

#endif // SERVER_H 