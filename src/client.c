#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

void send_get_request(int sock);
void send_post_request(int sock);
void send_put_request(int sock);

int main(int argc, char *argv[]){

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    if (sockfd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server\n");
    printf("Do you have an account? (y/n): ");
    char response[5];
    fgets(&response, 5, stdin);
    if(response[0] == 'y'){
        response[4] = '\0'; //remove newline character
        send_get_request(sockfd);
    }
    else{
        send_put_request(sockfd);
    }


    return 0;
}

void send_get_request(int sock){
    char *request_temp = "GET /";
    char nickname[20];
    printf("Enter the nickname of the account you want to access: ");
    fgets(nickname, 20, stdin);
    printf("You entered: %s\n", nickname);
    char *request = (char *)malloc(strlen(request_temp) + strlen(nickname) + 1);
    strcat(request, request_temp);
    strcat(request, nickname);
    request[strlen(request)-1] = '\0'; //remove newline character from input
    send(sock, request, strlen(request), 0);
    free(request);
}

void send_post_request(int sock){

}
void send_put_request(int sock){

}