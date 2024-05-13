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

volatile int data_ready = 0;


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

void handle_login(int socket, char buffer[], struct server_bank *server_bank);
void handle_create_acc(int socket, char buffer[], struct server_bank *server_bank);

void handle_get_request(int socket, char buffer[], struct server_bank *server_bank);
void handle_put_request(int socket, char buffer[], struct server_bank *server_bank);
void handle_request(int socket, struct server_bank *server_bank);
void send_response(int sock, char *request);

void sigsusr1_send(int pid);
void sigchld_handler(int signum);
void sigusr1_handler(int signum);

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
    sa_usr1.sa_handler = &sigusr1_handler;
    sa_usr1.sa_flags = SA_RESTART;
    if(sigaction(SIGUSR1, &sa_usr1, NULL) == -1){
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    struct server_bank *server_bank = malloc(sizeof(struct server_bank));
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
                close(new_connection_socket);
            }
        }
        else{
            perror("fork");
            exit(EXIT_FAILURE);
        }

    }

    return 0;
}

void handle_request(int socket, struct server_bank *server_bank){
    
    char buffer[1024] = {0};
    read(socket, buffer, 1024);

    char method[10] = {0};
    char path[500] = {0};

    sscanf(buffer, "%s %s", method, path);
    printf("Method: %s\n", method);
    printf("Path: %s\n", path);

    if(strstr(method, "GET") != NULL){
        handle_get_request(socket, path, server_bank);
    }
    else if(strstr(method, "PUT") != NULL){
        handle_put_request(socket, path, server_bank);
    }
    else{
        printf("Invalid request\n");
    }
}


void handle_get_request(int socket, char buffer[], struct server_bank *server_bank){
    char *login = "login";
    char *withdraw = "withdraw";
    char *balance = "balance";

    if(strstr(buffer, login) != NULL){
        handle_login(socket, buffer, server_bank);
    }
    else if(strstr(buffer, balance) != NULL){
        fetch_balance(socket, buffer, server_bank);
    }
    else if (strstr(buffer, withdraw) != NULL){
        withdraw_money(socket, buffer, server_bank);
    }
    
    else{
        printf("Invalid request\n");
    }
}

void handle_put_request(int socket, char buffer[], struct server_bank *server_bank){
    char *create = "create";
    char *deposit = "deposit";
    if(strstr(buffer, create) != NULL){
        handle_create_acc(socket, buffer, server_bank);
    }
    else if (strstr(buffer, deposit) != NULL){
        deposit_money(socket, buffer, server_bank);
    }
    else{
        printf("Invalid request\n");
    }
}



void handle_login(int socket, char buffer[], struct server_bank *server_bank){
    char nickname[20];
    char password[20];
    char *token= strtok(buffer, " ");
    // token = strtok(NULL, "/");
    // strcpy(nickname, token);
    // token = strtok(NULL, "/");
    // strcpy(password, token);

    strcpy(server_bank->buffer, buffer);
    send_to_bank(server_bank);

    while(1){
        if(data_ready == 1){
            read_from_bank(server_bank);
            data_ready = 0;
            if(strstr(server_bank->buffer, "404")){
                char *response = "404 NOT FOUND";
                send_response(socket, response);
            }
            else{
                char *response = "200 OK";
                send_response(socket, response);
            }
            break;
        }
        else{
            continue;
        }
    }

    
    printf("Logging in with nickname: %s\n", nickname);
    printf("Password: %s\n", password);
}

void handle_create_acc(int socket, char buffer[], struct server_bank *server_bank){
    char nickname[20];
    char password[20];
    char *token= strtok(buffer, "/");
    token = strtok(NULL, "/");
    strcpy(nickname, token);
    token = strtok(NULL, "/");
    strcpy(password, token);

    strcpy(server_bank->buffer, buffer);
    send_to_bank(server_bank);

    while(1){
        if(data_ready == 0){
            continue;
        }
        else{
            read_from_bank(server_bank);
            printf("Creating account with nickname: %s\n", nickname);
            printf("Password: %s\n", server_bank->buffer);
            data_ready = 0;
            break;
        }
    }
}

void deposit_money(int socket, char buffer[], struct server_bank *server_bank){

}

void fetch_balance(int socket, char buffer[], struct server_bank *server_bank){

}

void withdraw_money(int socket, char buffer[], struct server_bank *server_bank){

}

void send_to_bank(struct server_bank *server_bank){
    sem_wait(server_bank->sem_server);
    strcpy(server_bank->shared_mem, server_bank->buffer);
    printf("SENDING TO BANK: %s\n", server_bank->shared_mem);
    sem_post(server_bank->sem_bank);
    sem_post(server_bank->sem_bank);
    sigsusr1_send(server_bank->bank_pid);
}

void read_from_bank(struct server_bank *server_bank){
    sem_wait(server_bank->sem_server);
    strcpy(server_bank->buffer, server_bank->shared_mem);
    printf("READING FROM BANK: %s\n", server_bank->buffer);
}

void send_response(int sock, char *request){
    send(sock, request, strlen(request), 0);
}

void sigusr1_handler(int signo){
    data_ready = 1;
}


void sigsusr1_send(int pid){
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