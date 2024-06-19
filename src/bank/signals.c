#include "../../include/signals.h"

void init_signal(struct sigaction *act, int signum, int flags, void (*handler)(int)){
    memset(act, 0, sizeof(struct sigaction));
    act->sa_handler = handler;
    act->sa_flags = flags;
    if(sigaction(signum, act, NULL) == -1){
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}
