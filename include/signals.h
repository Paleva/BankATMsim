#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void init_signal(struct sigaction *act, int signum, int flags, void (*handler)(int));

#endif