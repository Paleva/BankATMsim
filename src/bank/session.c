#include "../../include/session.h"

struct session *get_session(struct session *sessions, int session_amount, pid_t connection_id){
    for(int i = 0; i < session_amount; i++){
        if(sessions[i].connection_id == connection_id){
            return &sessions[i];
        }
    }
    return NULL;
}
struct session *push_session(struct session *sessions, int current_account, pid_t connection_id, int session_amount, char *shared_mem){
    sessions = realloc(sessions, (session_amount + 1) * sizeof(struct session));
    if(sessions == NULL){
        perror("Memory allocation");
        exit(EXIT_FAILURE);
    }
    sessions[session_amount].current_account = current_account;
    sessions[session_amount].connection_id = connection_id;
    sessions[session_amount].shared_mem_ptr = shared_mem;
    return sessions;
}
