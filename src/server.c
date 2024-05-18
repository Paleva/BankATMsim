#include "../include/server.h"

sig_atomic_t data_ready = 0;
sig_atomic_t exit_flag = 0;

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

    struct sigaction sa_int;
    bzero(&sa_int, sizeof(sa_int));
    sa_int.sa_handler = &sigint_handler;
    if(sigaction(SIGINT, &sa_int, NULL) == -1){
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    struct server_bank *server_bank = malloc(sizeof(struct server_bank));
    server_bank->sem_bank = semaphore_open(SEMKEYBANK);
    server_bank->sem_server = semaphore_open(SEMKEYSERVER);
    server_bank->bank_pid = getppid();

    int new_connection_socket;
    int addrlen = sizeof(serv_addr);
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
                int shmid = shared_mem_id(getpid());
                server_bank->shared_mem = shared_mem_ptr(shmid);
                close(sockfd);
                int request = 0;
                while(1){
                    if(request == 0){
                        request = handle_request(new_connection_socket, server_bank);
                    }
                    else if(request < 0 || exit_flag > 0){
                        send_response(new_connection_socket, "200 OK");
                        close(new_connection_socket);
                        sem_close(server_bank->sem_bank);
                        sem_close(server_bank->sem_server);
                        sem_destroy(server_bank->sem_bank);
                        sem_destroy(server_bank->sem_server);
                        shmctl(shmid, IPC_RMID, NULL);
                        shmdt(server_bank->shared_mem);
                        free(server_bank);
                        printf("Connection closed\n");
                        exit(EXIT_SUCCESS);
                        
                    }
                    else{
                        send_response(new_connection_socket, "400 BAD REQUEST");
                        continue;
                    }
                }
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


int handle_request(int socket, struct server_bank *server_bank){
    
    char buffer[1024] = {0};
    read(socket, buffer, 1024);
    char method[10] = {0};
    char path[500] = {0};

    sscanf(buffer, "%s %s", method, path);
    printf("Method: %s\n", method);
    printf("Path: %s\n", path);

    if(strstr(method, "GET") != NULL){
        handle_get_request(socket, path, server_bank);
        return 0;
    }
    else if(strstr(method, "PUT") != NULL){
        handle_put_request(socket, path, server_bank);
        return 0;
    }
    else if(strstr(method, "EXIT") != NULL){
        return -1;
    }
    else{
        printf("Invalid request\n");
        return 1;
    }
    return 0;
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
    char nickname[20] = {0};
    char password[20] = {0};
    char *token= strtok(buffer, " ");
    printf("BUFFER: %s\n", buffer);
    strcpy(server_bank->buffer, buffer);
    send_to_bank(server_bank);

    token = strtok(buffer, "/");
    token = strtok(NULL, "/");
    token = strtok(NULL, "/");
    strcpy(password, token);

    while(1){
        if(data_ready == 1){
            read_from_bank(server_bank);
            data_ready = 0;
            int status;
            char *token_ = strtok(server_bank->buffer, " ");
            status = atoi(token_);
            if(status == 200){
                token_ = strtok(NULL, " ");
                token_ = strtok(token_, "/");
                token_ = strtok(NULL, "/");
                char pass[20] = {0};
                strcpy(pass, token_);
                if(strcmp(pass, password) == 0){
                    char *response = "200 OK";
                    send_response(socket, response);
                }
                else{
                    char *response = "401 UNAUTHORIZED";
                    send_response(socket, response);
                }
            }
            else{
                char *response = "404 NOTFOUND";
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

    strcpy(server_bank->buffer, buffer);
    send_to_bank(server_bank);

    while(1){
        if(data_ready == 1){
            read_from_bank(server_bank);
            data_ready = 0;
            if(strstr(server_bank->buffer, "201")){
                char *response = "201 CREATED";
                send_response(socket, response);
            }
            else{
                char *response = "400 BAD REQUEST";
                send_response(socket, response);
            }
            break;
        }
        else{
            continue;
        }
    }
}
void deposit_money(int socket, char buffer[], struct server_bank *server_bank){

    strcpy(server_bank->buffer, buffer);
    send_to_bank(server_bank);

    while(1){
        if(data_ready == 1){
            read_from_bank(server_bank);
            data_ready = 0;
            if(strstr(server_bank->buffer, "202")){
                char *response = "202 ACCEPTED";
                send_response(socket, response);
            }
            else{
                char *response = "400 BAD REQUEST";
                send_response(socket, response);
            }
            break;
        }
        else{
            continue;
        }
    }
}

void fetch_balance(int socket, char buffer[], struct server_bank *server_bank){
    
    strcpy(server_bank->buffer, buffer);
    send_to_bank(server_bank);

    while(1){
        if(data_ready == 1){
            read_from_bank(server_bank);
            data_ready = 0;
            if(strstr(server_bank->buffer, "200")){
                send_response(socket, server_bank->buffer);
            }
            else{
                char *response = "400 BAD REQUEST";
                send_response(socket, response);
            }
            break;
        }
        else{
            continue;
        }
    }
}

void withdraw_money(int socket, char buffer[], struct server_bank *server_bank){
    
    strcpy(server_bank->buffer, buffer);
    send_to_bank(server_bank);

    while(1){
        if(data_ready == 1){
            read_from_bank(server_bank);
            data_ready = 0;
            if(strstr(server_bank->buffer, "202")){
                char *response = "202 ACCEPTED";
                send_response(socket, response);
            }
            else{
                char *response = "400 BAD REQUEST";
                send_response(socket, response);
            }
            break;
        }
        else{
            continue;
        }
    }
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

void sigsusr1_send(int pid){
    kill(pid, SIGUSR1);
}

void send_response(int sock, char *request){
    send(sock, request, strlen(request), 0);
}

void sigusr1_handler(int signo){
    data_ready = 1;
}

void sigchld_handler(int signo){
    while(waitpid(-1, NULL, WNOHANG) > 0); // figure how this works
}

void sigint_handler(int signum){
    exit_flag = signum;
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