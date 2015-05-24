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
 * @brief Is the action done by the main thread, which is waiting for the child processes (clients) to finish
 * @return - exit status for the main process
 */
int parent_action();

/*
 * @brief Is the action of the child processes (clients), which is choosing the best balcao, writing their fifo on it's fifo and wait for response
 * @param shname name of the shared memory
 * @param key shared memory key
 * @return exit status for the child process (client)
 */
int child_action(char *shname, int key);

/*
 * @brief Remaps the shared memory to the child processe's virtual memory
 * @param shmem_name name of the shared memory
 * @param key shared memory key
 * @return 0 on success, otherwise means errors occurred
 */
shop_t *child_remap_shmem(char* shmem_name, int key);

/*
 * @brief Chooses the balcao with less clients in attendance and stores it's index in min_occup_index
 * @param shop pointer to the shop "object"
 * @param num_balcoes number of active counters in the shop
 * @param min_occup_index pointer to the variable where the desired index is to be stored
 * @return no value is stored if there are no active counters
 */
void choose_best_balcao(shop_t *shop, const int num_balcoes, size_t *min_occup_index);

/*
 * @brief Retrieves the shop "object" from the shared memory
 * @param shop pointer where the shop must be stored
 * @param key pointer to the shared memory's key
 * @param shm_name name of the shared memory
 * @return 0 on success, otherwise means errors occurred
 */
int retrieve_shop(shop_t *shop, int *key, char* shm_name);

/*
 * @brief Builds a string with the required fifo pathname format
 * @param pid PID of the program
 * @return fifo pathname in specified format
 */
char *get_fifo_pathname(int pid);

#endif
