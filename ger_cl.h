#ifndef GER_CL_H_
#define GER_CL_H_

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "utils.h"

#define SHM_SIZE 1024*1024	// defines the shared memory as a 1MB region

/*
 * @return - success(0), invalid arguments(1), internal error like fork(2), child-process error(3)
 */
int main(int argc, char **argv);


#endif
