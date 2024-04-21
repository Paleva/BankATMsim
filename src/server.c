#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>
#include <bits/sigaction.h>
#include <asm-generic/signal-defs.h>

#define PORT 8080
#define MAX_CONNECTIONS 5


void handle_request(int socket);

void handle_get_request(int socket);
void handle_post_request(int socket);
void handle_put_request(int socket);
void sent_get_request(int sock);
void sigchld_handler(int signo);

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
    serv_addr.sin_addr.s_addr = INADDR_ANY;

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
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &sa, NULL) == -1){
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    
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
    char *GET = "GET";
    char *POST = "POST";
    char *PUT = "PUT";
    read(socket, buffer, 1024);
    if(strstr(buffer, GET) != NULL){
        handle_get_request(socket);
    }
    else if(strstr(buffer, POST) != NULL){
        handle_post_request(socket);
    }
    else if(strstr(buffer, PUT) != NULL){
        handle_put_request(socket);
    }
    else{
        printf("Invalid request\n");
    }
}

void sigchld_handler(int signo){
    while(waitpid(-1, NULL, WNOHANG) > 0);
}


void handle_get_request(int socket){
    char buffer[1024] = {0};
    char *response = "POST /users HTTP/1.1\nContent-Type: application/json\n\n{\"name\": \"John Doe\"}";
    read(socket, buffer, 1024);
    send(socket, response, strlen(response), 0);
    printf("%s\n", buffer);

}


void handle_post_request(int socket){
    
}
void handle_put_request(int socket){

}
void sent_get_request(int sock){

}