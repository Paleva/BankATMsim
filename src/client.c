#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

void login_into_acc(int sock);
void create_acc(int sock);

int send_get_request(int sock, char *request);
int send_post_request(int sock);
int send_put_request(int sock, char *request);

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
    }
    else{
        create_acc(sockfd);
    }


    return 0;
}

int send_get_request(int sock, char *request){
    send(sock, request, strlen(request), 0);
    return 0;
}

int send_put_request(int sock, char *request){
    send(sock, request, strlen(request), 0);    
    return 0;
}


void login_into_acc(int sock){
    char *request_temp = "GET /login:";
    char nickname[20];
    char password[20];

    printf("Enter the nickname of the account you want to access: ");
    fgets(nickname, 20, stdin);
    
    char request[1024] = {0};
    strcpy(request, request_temp);
    strcat(request, nickname);
    request[strlen(request)-1] = '\0';
    printf("Loging into account: %s\n", nickname);
    printf("Enter password: ");
    fgets(password, 20, stdin);
    printf("You entered: %s\n", request);
    strcat(request, ":");
    strcat(request, password);
    send_get_request(sock, request);
}

void create_acc(int sock){
    char *request_temp = "PUT /create:";
    char nickname[20];

    while(1){
        printf("Enter the nickname of the account you want to create: ");
        fgets(nickname, 20, stdin);
        if(strcmp(nickname, "\n") == 0){
            printf("Invalid nickname\n");
        }
        else{
            nickname[strlen(nickname)-1] = '\0';
            break;
        }
    }

    printf("You entered: %s\n", nickname);

    char *request = (char *)malloc(strlen(request_temp) + strlen(nickname) + 1);
    strcpy(request, request_temp);
    strcat(request, nickname);
    
    send_put_request(sock, request);
    
    free(request);
}