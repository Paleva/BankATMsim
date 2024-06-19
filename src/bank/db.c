#include "../../include/db.h"

// updates the file with the accounts
void update_db(struct account *accounts, int account_number){
    FILE *file;
    file = fopen(ACCOUNTS, "w");
    if(file == NULL){
        perror("Opening file");
        exit(EXIT_FAILURE);
    }
    int i;
    for(i = 0; i < account_number; i++){
        fprintf(file, "%s:%s:%ld\n", accounts[i].nickname, accounts[i].password, accounts[i].balance);
    }
    fclose(file);
}  
// reads accounts from the file
struct account *read_accounts(int *account_number){
    FILE *file;
    file = fopen(ACCOUNTS, "r");
    if(file == NULL){
        perror("Opening file");
        exit(EXIT_FAILURE);
    }

    char line[1024];
    struct account *accounts = NULL;
    //read "database" into memory
    while(fgets(line, 1024, file) != NULL){
        char *nickname = strtok(line, ":");
        char *password = strtok(NULL, ":");
        char *balance = strtok(NULL, ":");
        accounts = push_account(accounts, nickname, password, atoll(balance), account_number);
    }
    fclose(file);
    return accounts;
}
struct account *push_account(struct account *accounts, char nickname[], char password[], int balance, int *account_number){
    accounts = realloc(accounts, ((*account_number) + 1) * sizeof(struct account));
    if(accounts == NULL){
        perror("Memory allocation");
        exit(EXIT_FAILURE);
    }
    strcpy(accounts[*account_number].nickname, nickname);
    strcpy(accounts[*account_number].password, password);
    accounts[*account_number].balance = balance;
    (*account_number)++;
    return accounts;
}

