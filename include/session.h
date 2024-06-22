#ifndef SESSION_H
#define SESSION_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

struct session{
    int current_account;
    pid_t connection_id;
    int shmid;
    char* shared_mem_ptr;
};

struct session *get_session(struct session *sessions, int session_amount, pid_t connection_id);
// pushes a session to the array
struct session *push_session(struct session *sessions, int current_account, pid_t connection_id, int session_amount, char *shared_mem);

#endif // SESSION_H