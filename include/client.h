#ifndef CLIENT_H
#define CLIENT_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080



void login_into_acc(int sock);
void create_acc(int sock);
void deposit_money(int sock);
void withdraw_money(int sock);
void fetch_balance(int sock);

void close_connection(int sock);

int send_request(int sock, char *request);

int handle_response(int sock, char buf[]);

#endif // CLIENT_H
