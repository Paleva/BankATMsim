#ifndef SHARED_MEM_H
#define SHARED_MEM_H

#include <stdlib.h>
#include <sys/shm.h>

int shared_mem_id(int key);
char *shared_mem_ptr(int key);


#endif // SHARED_MEM_H