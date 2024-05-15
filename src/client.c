#include "../include/client.h"

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

    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        // perror("connect");
        printf("Failed to connect. Try again please\n");
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server\n");
    printf("Do you have an account? (y/n): ");
    char response[5] = {0};
    fgets(response, 5, stdin);

    if(response[0] == 'y'){
        login_into_acc(sockfd);
        printf("You are now logged in\n");
        if(handle_response(sockfd) == 200){
            printf("What do you want to do?\n");
            printf("1. Deposit money\n");
            printf("2. Withdraw money\n");
            printf("3. Check balance\n");
            printf("4. Exit\n");
            do{
                printf("Enter your choice: ");
                fgets(response, 5, stdin);
                if(response[0] == '1'){
                    deposit_money(sockfd);
                    if(handle_response(sockfd) == 202){
                        printf("Money deposited\n");
                    }
                    else{
                        printf("Error while depositing\n");
                    }
                }
                else if(response[0] == '2'){
                    withdraw_money(sockfd);
                }
                else if(response[0] == '3'){
                    fetch_balance(sockfd);
                }
                else if(response[0] == '4'){
                    send_request(sockfd, "exit");
                    if(handle_response(sockfd) == 200){
                        close_connection(sockfd);
                        printf("Connection closed\n");
                        exit(EXIT_SUCCESS);
                    }
                }
                else{
                    printf("Invalid input\n");
                }
                printf("Do you want to continue:\n");
            }while(1);

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
        if(handle_response(sockfd) == 201){
            printf("Account created succesfully\n");
            printf("Please connect again to login\n");
            close_connection(sockfd);
            exit(EXIT_SUCCESS);
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
    // printf("Response: %s\n", buffer);
    char code[4];
    char response[1024];
    sscanf(buffer, "%s %s", code, response);
    // printf("Code: %s\n", code);
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
    nickname[strlen(nickname)-1] = '\0';

    printf("Enter password: ");
    fgets(password, 20, stdin);

    strcpy(request, request_temp);
    strcat(request, nickname);
    strcat(request, "/");
    strcat(request, password);

    request[strlen(request)-1] = '\0';
    
    printf("You entered: %s\n", nickname);
    send_request(sock, request);

}

void deposit_money(int sock){
    char *request_temp = "PUT /deposit/";
    char request[1024];
    char amount_to_deposit[20];
    printf("Enter the amount you want to deposit: ");
    fgets(amount_to_deposit, 20, stdin);
    amount_to_deposit[strlen(amount_to_deposit)-1] = '\0';
    strcpy(request, request_temp);
    strcat(request, amount_to_deposit);
    send_request(sock, request);
}

void withdraw_money(int sock){
    char *request_temp = "GET /withdraw/";
}

void fetch_balance(int sock){
    char *request_temp = "GET /balance/";

}

void close_connection(int sock){
    char *request_temp = "GET /exit/";
}
