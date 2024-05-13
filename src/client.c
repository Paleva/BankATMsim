#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080



void login_into_acc(int sock);
void create_acc(int sock);

int send_request(int sock, char *request);

int handle_response(int sock);

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

    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server\n");
    printf("Do you have an account? (y/n): ");
    char response[5];
    fgets(response, 5, stdin);
    if(response[0] == 'y'){
        response[4] = '\0'; //remove newline character
        login_into_acc(sockfd);
        if(handle_response(sockfd) == 200){
            printf("You are now logged in\n");
        }
        else if(handle_response(sockfd) == 404){
            printf("Account not found\n");
        }
        else{
            printf("Error\n");
        }
        
    }
    else{
        create_acc(sockfd);
        handle_response(sockfd);
        if(handle_response(sockfd) == 201){
            printf("Account created\n");
        }
    }


    return 0;
}

int send_request(int sock, char *request){
    send(sock, request, strlen(request), 0);    
    return 0;
}

int handle_response(int sock){
    char buffer[1024] = {0};
    read(sock, buffer, 1024);

    char code[4];
    char response[1024];
    sscanf(buffer, "%s %s", code, response);

    return atoi(code);
}


void login_into_acc(int sock){
    char *request_temp = "GET /login/";
    char request[1024];
    char nickname[20];
    char password[20];
    
    printf("Enter the nickname of the account you want to access: ");
    fgets(nickname, 20, stdin);
    nickname[strlen(nickname)-1] = '\0';

    printf("Enter password: ");
    fgets(password, 20, stdin); 

    strcpy(request, request_temp);
    strcat(request, nickname);
    strcat(request, "/");
    strcat(request, password);

    request[strlen(request)-1] = '\0';
    
    printf("You entered: %s\n", request);
    
    send_request(sock, request);
}

void create_acc(int sock){
    char *request_temp = "PUT /create/";
    char request[1024];
    char nickname[20];
    char password[20];

    printf("Enter the nickname of the account you want to create: ");
    fgets(nickname, 20, stdin);

    printf("Enter password: ");
    fgets(password, 20, stdin);

    strcpy(request, request_temp);
    strcat(request, nickname);
    strcat(request, "/");
    strcat(request, password);

    request[strlen(nickname)-1] = '\0';
    
    printf("You entered: %s\n", nickname);
    send_request(sock, request);

}