#include "../../include/client.h"

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
        printf("Failed to connect. Try again please\n");
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server\n");
    printf("Do you have an account? (y/n): ");
    char response[5] = {0};
    fgets(response, 5, stdin);

    while(1){
        int status = 0;
        if(response[0] == 'y'){
            login_into_acc(sockfd);
            status = handle_response(sockfd, NULL);
            if(status == 200){
                printf("You are now logged in\n");
                printf("What do you want to do?\n");
                printf("1. Deposit money\n");
                printf("2. Withdraw money\n");
                printf("3. Check balance\n");
                printf("4. Exit\n");
                while(1){
                    printf("Enter your choice: ");
                    fgets(response, 5, stdin);
                    if(response[0] == '1'){
                        deposit_money(sockfd);
                        char buff[1024];
                        status = handle_response(sockfd, buff);
                        if(status == 202){
                            printf("Money deposited\n");
                        }
                        else{
                            printf("Error while depositing\n");
                        }
                    }
                    else if(response[0] == '2'){
                        withdraw_money(sockfd);
                        char buff[1024];
                        status = handle_response(sockfd, buff);
                        if(status == 202){
                            printf("Money withdrawn\n");
                        }
                        else{
                            printf("Error while withdrawing. Can't withdraw more than you have\n");
                        }
                    }
                    else if(response[0] == '3'){
                        fetch_balance(sockfd);
                        char buff[1024];
                        status = handle_response(sockfd, buff);
                        char *token = strtok(buff, "/");
                        token = strtok(NULL, "/");
                        if(status == 200){
                            printf("Your remaining balance: %s\n", token);
                        }
                        else{
                            printf("Error while fetching balance\n");
                        }
                    }
                    else if(response[0] == '4'){
                        send_request(sockfd, "EXIT /exit/");
                        status = handle_response(sockfd, NULL);
                        if(status == 200){
                            printf("Connection closed\n");
                            close(sockfd);
                            exit(EXIT_SUCCESS);
                        }
                    }
                    else{
                        printf("Invalid input\n");
                    }
                }
            }
            else if(status == 404){
                printf("Account not found\n");
                printf("Do you want to create an account? (y/n): ");
                fgets(response, 5, stdin);
            }
            else if(status == 401){
                printf("Invalid password\n");
            }
            else{
                close_connection(sockfd);
                exit(EXIT_SUCCESS);
                printf("Error\n");
            }
        }
        else{
            create_acc(sockfd);
            status = handle_response(sockfd, NULL);
            if(status == 201){
                
                printf("Account created succesfully\n");
                printf("Please connect again to login\n");
                close_connection(sockfd);
                exit(EXIT_SUCCESS);
            }
        }
    }
    return 0;
}

int send_request(int sock, char *request){
    send(sock, request, strlen(request), 0);
    return 0;
}

int handle_response(int sock, char buf[]){
    char buffer[1024] = {0};
    read(sock, buffer, 1024);
    // printf("Response: %s\n", buffer);
    char code[4];
    char response[1024];
    sscanf(buffer, "%s %s", code, response);

    if(buf != NULL){
        strcpy(buf, response);
    }

    return atoi(code);
}


void login_into_acc(int sock){
    char *request_temp = "GET /login/";
    char request[1024];
    char nickname[20];
    char password[20];
    
    do{
        printf("Enter the nickname of the account you want to access: ");
        fgets(nickname, 20, stdin);
    }while(strlen(nickname) <= 1);

    nickname[strlen(nickname)-1] = '\0';

    do{
        printf("Enter password: ");
        fgets(password, 20, stdin);
        remove_whitespaces(password);
    }while(strlen(password) <= 1);
    
    strcpy(request, request_temp);
    strcat(request, nickname);
    strcat(request, "/");
    strcat(request, password);

    request[strlen(request)-1] = '\0';
    
    // printf("You entered: %s\n", request);
    
    send_request(sock, request);
}

void create_acc(int sock){
    char *request_temp = "PUT /create/";
    char request[1024];
    char nickname[20];
    char password[20];

    do{
        printf("Enter the nickname of the account you want to create: ");
        fgets(nickname, 20, stdin);
        remove_whitespaces(nickname);
    }while (strlen(nickname) <= 1);

    nickname[strlen(nickname)-1] = '\0';

    do{
        printf("Enter password: ");
        fgets(password, 20, stdin);
        remove_whitespaces(password);
    }while(strlen(password) <= 1);
    
    strcpy(request, request_temp);
    strcat(request, nickname);
    strcat(request, "/");
    strcat(request, password);

    request[strlen(request)-1] = '\0';
    
    // printf("You entered: %s\n", nickname);
    
    send_request(sock, request);
}

void deposit_money(int sock){
    char *request_temp = "PUT /deposit/";
    char request[1024];
    char amount_to_deposit[20];

    do{
        printf("Enter the amount you want to deposit: ");
        fgets(amount_to_deposit, 20, stdin);
        remove_whitespaces(amount_to_deposit);
    }while(check_if_contains_letters(amount_to_deposit) || strlen(amount_to_deposit) <= 1 || amount_to_deposit[0] == '-');
    
    amount_to_deposit[strlen(amount_to_deposit)-1] = '\0';

    strcpy(request, request_temp);
    strcat(request, amount_to_deposit);
    send_request(sock, request);
}

void withdraw_money(int sock){
    char *request_temp = "GET /withdraw/";
    char request[1024];
    char amount_to_withdraw[20];

    do{
        printf("Enter the amount you want to withdraw: ");
        fgets(amount_to_withdraw, 20, stdin);
        remove_whitespaces(amount_to_withdraw);
    }while(check_if_contains_letters(amount_to_withdraw) || strlen(amount_to_withdraw) == 0);
    
    amount_to_withdraw[strlen(amount_to_withdraw)-1] = '\0';
    
    strcpy(request, request_temp);
    strcat(request, amount_to_withdraw);
    send_request(sock, request);
}

void fetch_balance(int sock){
    char *request_temp = "GET /balance/";
    send_request(sock, request_temp);
}

void close_connection(int sock){
    char *request_temp = "EXIT /exit/";
    send_request(sock, request_temp);
}

int check_if_contains_letters(char *str){
    for(int i = 0; i < strlen(str); i++){
        if(isalpha(str[i])){
            return 1;
        }
    }
    return 0;
}
void remove_whitespaces(char *str){
    for(int i = 0; i < strlen(str); i++){
        if(str[i] == ' '){
            str[i] = '\0';
        }
    }
}