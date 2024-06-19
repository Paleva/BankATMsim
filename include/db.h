#ifndef DB_H
#define DB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define ACCOUNTS "db/accounts.txt"

struct account {
    char nickname[20];
    char password[20];
    int64_t balance;
};

void update_db(struct account *accounts, int account_number);
struct account *read_accounts(int *account_number);
struct account *push_account(struct account *accounts, char nickname[], char password[], int balance, int *account_number);


#endif