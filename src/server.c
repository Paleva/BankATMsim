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

volatile server_pid = 0;

semaphore *semaphore_open(char* name);

void fetch_balance(); // fetch balance from bank using pipe probs

void handle_request(int socket);

int shared_mem_id(int key);
char *shared_mem_ptr(int key);

void handle_login(int socket, char buffer[]);
void handle_create_acc(int socket, char buffer[]);

void handle_get_request(int socket, char buffer[]);
void handle_post_request(int socket);
void handle_put_request(int socket, char buffer[]);

void send_get_request(int sock, char *request);


void sigchld_handler(int signo);

void sigsusr1_send(int pid);

void sigsusr1_receive(int sig, siginfo_t *info, void *ucontext, int *server_ready);

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
    sigaction(SIGCHLD, &sa, NULL);

    if(sigaction(SIGCHLD, &sa, NULL) == -1){
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    
    struct sigaction sa_usr1;
    bzero(&sa_usr1, sizeof(sa_usr1));
    sa_usr1.sa_sigaction = &sigsusr1_receive;
    sa_usr1.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGUSR1, &sa_usr1, NULL);

    int shmid = shared_mem_id(SHMKEY);
    char *shmptr = shared_mem_ptr(shmid);
    semaphore *sem_bank = semaphore_open(SEMKEYBANK);
    semaphore *sem_server = semaphore_open(SEMKEYSERVER);

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
                handle_request(new_connection_socket);
                close(new_connection_socket);
                exit(0);
            }
            else{
                close(new_connection_socket); // close new socket because parent process does not need it
                // waitpid(pid, NULL, 0);
                printf("IM PARENT\n PID: %d \n", getpid());
            }
        }
        else{
            perror("fork");
            exit(EXIT_FAILURE);
        }

    }

    return 0;
}

void handle_request(int socket){
    
    char buffer[1024] = {0};
    bzero(buffer, 1024);
    read(socket, buffer, 1024);

    char method[10] = {0};
    char path[500] = {0};

    sscanf(buffer, "%s %s", method, path);
    printf("Method: %s\n", method);
    printf("Path: %s\n", path);

    if(strstr(method, "GET") != NULL){
        handle_get_request(socket, buffer);
    }
    else if(strstr(method, "POST") != NULL){
        // handle_post_request(socket);
    }
    else if(strstr(method, "PUT") != NULL){
        handle_put_request(socket, buffer);
    }
    else{
        printf("Invalid request\n");
    }
}


void handle_get_request(int socket, char buffer[]){
    char *login = "login";
    char *balance = "balance";

    if(strstr(buffer, login) != NULL){
        handle_login(socket, buffer);
    }
    else if(strstr(buffer, balance) != NULL){
        //send balance
    }
    else{
        printf("Invalid request\n");
    }
}

void handle_put_request(int socket, char buffer[]){
    char *create = "create";
    char *deposit = "deposit";
    if(strstr(buffer, create) != NULL){
        handle_create_acc(socket, buffer);
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


void handle_login(int socket, char buffer[]){
    char *pass[100] = {0};
    char *nickname = strtok(buffer, ":");
    nickname = strtok(NULL, ":");
    
    int return_code = 0;

    if (return_code == 0){
        write(socket, "Enter your password:", 21);
    }
    else if (return_code == 1)
    {
        write(socket, "Account does not exist", 22);
    }
    
    printf("Loging into account: %s\n", nickname);


    printf("Enter password: ");
    fgets(pass, 100, stdin);
    char *response = "Logged in successfully";
    send_get_request(socket, response);
}

void handle_create_acc(int socket, char buffer[]){
    char *nickname = strtok(buffer, ":");
    nickname = strtok(NULL, ":");
    printf("Creating account with nickname: %s\n", nickname);
    //request password

    //create account with bank thru pipes or shared memory
    //send response
    char *response = "Account created";
    send_get_request(socket, response);
}



void sigsusr1_send(int pid){
    kill(pid, SIGUSR1);
}

void sigchld_handler(int signo){
    while(waitpid(-1, NULL, WNOHANG) > 0); // figure how this works
}

void sigsusr1_receive(int sig, siginfo_t *info, void *ucontext, int *server_ready){
    server_pid = info->si_pid;
    *server_ready = 1;
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