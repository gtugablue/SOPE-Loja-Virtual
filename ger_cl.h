#ifndef GER_CL_H_
#define GER_CL_H_

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "utils.h"
#include "balcao.h"

#define CL_FIFO_NAME "/tmp/fc_"
#define CL_FIFO_MODE 0777

/*
 * @return - success(0), invalid arguments(1), internal error like fork(2), child-process error(3)
 */
int main(int argc, char **argv);

/*
 * @return - exit status for the main process
 */
int parent_action();

/*
 * @return exit status for the child process (client)
 */
int child_action();


#endif
