#ifndef GER_CL_H_
#define GER_CL_H_

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define SHM_SIZE 1024*1024	// defines the shared memory as a 1MB region


int main(int argc, char **argv);
int parse_int(int* dest, char* str, int base);	// returns 0 if str is a number, 1 otherwise


#endif
