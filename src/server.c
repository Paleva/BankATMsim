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

int data_ready = 0;

struct server_to_bank{
    int bank_pid;
    char *shared_mem;
    char *buffer;
    semaphore *sem_bank;
    semaphore *sem_server;
};

semaphore *semaphore_open(char* name);

// char* shared, char *str, semaphore *sem_bank, semaphore *sem_server
void send_to_bank(struct server_to_bank *server_bank);
void read_from_server(struct server_to_bank *server_bank);
void fetch_balance(); // fetch balance from bank using pipe probs

void handle_request(int socket, struct server_to_bank *server_bank);

int shared_mem_id(int key);
char *shared_mem_ptr(int key);

void handle_login(int socket, char buffer[], struct server_to_bank *server_bank);
void handle_create_acc(int socket, char buffer[], struct server_to_bank *server_bank);

void handle_get_request(int socket, char buffer[], struct server_to_bank *server_bank);
void handle_put_request(int socket, char buffer[], struct server_to_bank *server_bank);

void send_get_request(int sock, char *request);

void sigchld_handler(int signo);
void sigsusr1_handler(int signo);

void sigsusr1_send(int pid);


int main(){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;    
    if (sockfd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    // serv_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if(listen(sockfd, MAX_CONNECTIONS) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    int new_connection_socket;
    int addrlen = sizeof(serv_addr);
    
    //handle zombie processes
    struct sigaction sa;
    bzero(&sa, sizeof(sa));
    sa.sa_handler = &sigchld_handler;
    sa.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &sa, NULL) == -1){
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa_usr1;
    bzero(&sa_usr1, sizeof(sa_usr1));
    sa_usr1.sa_handler = &sigsusr1_handler;
    sa_usr1.sa_flags = SA_RESTART;
    if(sigaction(SIGUSR1, &sa_usr1, NULL) == -1){
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    
    struct server_to_bank *server_bank = malloc(sizeof(struct server_to_bank));
    int shmid = shared_mem_id(SHMKEY);
    server_bank->shared_mem = shared_mem_ptr(shmid);
    server_bank->sem_bank = semaphore_open(SEMKEYBANK);
    server_bank->sem_server = semaphore_open(SEMKEYSERVER);
    server_bank->bank_pid = getppid();

    printf("Server listening on port %d\n", PORT);
    while(1){
        //accept connection from client
        if((new_connection_socket = accept(sockfd, (struct sockaddr *)&serv_addr, (socklen_t*)&addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        //handle client request in a seperate process
        pid_t pid;
        pid = fork();
        if(pid >= 0){    
            if(pid == 0){
                close(sockfd); // close original socket because accept creates a new one
                printf("IM KID\n PID: %d \n", getpid());
                handle_request(new_connection_socket, server_bank);
                close(new_connection_socket);
                exit(0);
            }
            else{
                close(new_connection_socket); // close new socket because parent process does not need it
                printf("IM PARENT\n PID: %d \n", getpid());
            }
        }
        else{
            perror("fork");
            exit(EXIT_FAILURE);
        }

    }
    free(server_bank);
    return 0;
}

void handle_request(int socket, struct server_to_bank *server_bank){
    
    char buffer[1024] = {0};
    read(socket, buffer, 1024);

    char method[10] = {0};
    char path[500] = {0};

    sscanf(buffer, "%s %s", method, path);
    printf("Method: %s\n", method);
    printf("Path: %s\n", path);

    if(strstr(method, "GET") != NULL){
        handle_get_request(socket, buffer, server_bank);
    }
    else if(strstr(method, "PUT") != NULL){
        handle_put_request(socket, buffer, server_bank);
    }
    else{
        printf("Invalid request\n");
    }
}


void handle_get_request(int socket, char buffer[], struct server_to_bank *server_bank){
    char *login = "login";
    char *balance = "balance";

    if(strstr(buffer, login) != NULL){
        handle_login(socket, buffer, server_bank);
    }
    else if(strstr(buffer, balance) != NULL){
        //send balance
    }
    else{
        printf("Invalid request\n");
    }
}

void handle_put_request(int socket, char buffer[], struct server_to_bank *server_bank){
    char *create = "create";
    char *deposit = "deposit";
    if(strstr(buffer, create) != NULL){
        handle_create_acc(socket, buffer, server_bank);
    }
    else if (strstr(buffer, deposit) != NULL){
        //deposit money contact bank
    }
    else{
        printf("Invalid request\n");
    }
}

void send_get_request(int sock, char *request){
    send(sock, request, strlen(request), 0);
}


void handle_login(int socket, char buffer[], struct server_to_bank *server_bank){
    
    server_bank->buffer = malloc(1024);
    strcpy(server_bank->buffer, buffer);
    send_to_bank(server_bank);
    while(data_ready == 0);
    read_from_server(server_bank);
    printf("Data read in memory: %s\n", server_bank->buffer);
    // char *nickname = strtok(buffer, ":");
    // nickname = strtok(NULL, ":");
    // char *password = strtok(NULL, ":");
    // printf("Loging into account: %s\n", nickname);
    char *response = "Logged in successfully";
    send_get_request(socket, response);
}

void handle_create_acc(int socket, char buffer[], struct server_to_bank *server_bank){
    char *nickname = strtok(buffer, ":");
    nickname = strtok(NULL, ":");
    printf("Creating account with nickname: %s\n", nickname);
    int bank_pid = getppid();
    sigsusr1_send(bank_pid);

    //create account with bank thru pipes or shared memory
    //send response
    char *response = "Account created";
    send_get_request(socket, response);
}

void send_to_bank(struct server_to_bank *server_bank){
    printf("BANK PID: %d\n", server_bank->bank_pid);
    sem_wait(server_bank->sem_server);
    sigsusr1_send(server_bank->bank_pid);
    printf("SERVER: Data written in memory: %s \n", server_bank->buffer);
    strcpy(server_bank->shared_mem, server_bank->buffer);
    // bzero(server_bank->buffer, 1024);
    sem_post(server_bank->sem_bank);
}

void read_from_server(struct server_to_bank *server_bank){
    sem_wait(server_bank->sem_server);
    printf("SERVER: Data read from memory: %s\n", server_bank->shared_mem);
    strcpy(server_bank->buffer, server_bank->shared_mem);
    // bzero(server_bank->shared_mem, 1024);
    sem_post(server_bank->sem_bank);
}


void sigsusr1_handler(int signo){
    printf("GOTTEN:SER\n");
    data_ready = 1;
}

void sigsusr1_send(int pid){
    printf("SENT:SER\n");
    kill(pid, SIGUSR1);
}

void sigchld_handler(int signo){
    while(waitpid(-1, NULL, WNOHANG) > 0); // figure how this works
}

semaphore *semaphore_open(char *name){
    semaphore *sem = sem_open(name, 0);
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