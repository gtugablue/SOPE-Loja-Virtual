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
#include <limits.h>
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
int child_action(char *shname, int key);

int retrieve_shop(shop_t *shop, int *key, char* shm_name);

char *get_fifo_pathname(int pid);

shop_t *child_remap_shmem(char* shmem_name, int key);

int attempt_mutex_lock(pthread_mutex_t *mutex, char *name);

int attempt_mutex_unlock(pthread_mutex_t *mutex, char *name);

#endif
