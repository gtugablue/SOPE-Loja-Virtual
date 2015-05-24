/** @file */

#ifndef BALCAO_H
#define BALCAO_H

#include <pthread.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "utils.h"

/**Message to send to clients when finished attendance*/
#define ATTEND_END_MESSAGE "fim_atendimento"

/**Maximum length for a FIFO's name*/
#define MAX_FIFO_NAME_LEN 20
/**Read/Write/Execute permissions for the FIFOs created*/
#define BALCAO_FIFO_MODE 0600

/**Maximum number of balcoes on the shared memory*/
#define MAX_NUM_BALCOES 30

/**Directory for the balcao FIFO to be stored at*/
#define FIFO_DIR "/tmp/"

/*!@brief Represents a balcao, storing the important variables to attend clients and create balcao and loja statistics*/
typedef struct Balcao_t
{
	// Synchronizing variables
	pthread_mutex_t balcao_mutex;

	// Data variables
	int num;
	time_t abertura;
	time_t duracao;
	char fifo_name[MAX_FIFO_NAME_LEN];
	int clientes_em_atendimento;
	int clientes_atendidos;
	double atendimento_medio;
} balcao_t;

/*!@brief Represents a shop, storing the statistics variables, mutex and counters*/
typedef struct
{
	// Synchronizing variables
	pthread_mutex_t loja_mutex;

	// Data variables
	time_t opening_time;
	unsigned num_balcoes;
	unsigned num_balcoes_abertos; // for efficiency purposes
	balcao_t balcoes[MAX_NUM_BALCOES];
} shop_t;

/**Size for the shared memory corresponds to the size of shop_t since it's the only thing stored there*/
#define SHARED_MEM_SIZE	sizeof(shop_t)
/**Read/Write/Execute shared memory permissions*/
#define SHARED_MEM_MODE 0600

/*!@brief Auxiliar struct to pass some arguments to the countdown thread*/
typedef struct {
	int *curr_count;
	char *path;
	shop_t *shop;
} counter_thr_info;

/*!@brief Auxiliar struct to pass arguments to the thread that attends clients*/
typedef struct {
	int duration;
	time_t start_time;
	char *cl_fifo;
	const char *shname;
	shop_t *shop;
} attend_thr_info;

/*
 * @brief Organizes all the balcao actions
 * @return 0 on success, otherwise means errors occurred
 */
int main(int argc, char *argv[]);

/*
 * @brief Initializes most of the required variables for the program
 * @param shname shared memory name
 * @param shop pointer where the shop struct will be stored
 * @param shm_id pointer to integer where the shm id will be stored
 * @return 0 upon success, 1 otherwise
 */
int init(const char *shname, shop_t **shop, int *shm_id);

/*
 * @brief Creates the shared memory specified as argument
 * @param name name for the shared memory
 * @param shm_id pointer to integer where the memory's id will be stored
 * @return pointer to the shop_t struct stored in the shared memory
 */
shop_t *create_shared_memory(const char *name, int *shm_id);

/*
 * @brief Initializes the first shop_t "object"
 * @param shop pointer to the object to initialize
 */
void initialize_shop_st(shop_t *shop);

/*
 * @brief Creates the balcao_t object and adds it to the shop struct
 * @param shname name of the shared memory
 * @param shop pointer to the stored shop
 * @return 0 on success, otherwise means errors occurred
 */
int join_shmemory(const char *shname, shop_t **shop);


/*
 * @brief Thread to count the length of balcao opening
 * @param arg must be a counter_thr_info "object" with the parameters specified by this struct
 */
void *timer_countdown(void *arg);

/*
 * @brief Thread to attend to a client
 * @param arg must be a attend_thr_info "object" with the parameters specified by this struct
 */
void *attend_client(void *arg);

/*
 * @brief Causes a blocking reading on the balcao fifo, waiting for clients to write on it or the timer to end
 * @param fifo_fd descriptor for the balcao fifo
 * @param non_opt_args non optional console arguments
 * @param shop pointer to the shop_t "object"
 * @return 0 on success, otherwise means errors occurred
 */
int read_fifo(int fifo_fd, char** non_opt_args, shop_t *shop);


/*
 * @brief Initializes the log file
 * @param shared memory name
 * @return 0 on success, otherwise means errors occurred
 */
int initialize_log(const char *sh_name);

/*
 * @brief Virtually closes the balcao to new clients when the timer has ended, but allows for current attendances to terminate
 * @param shop pointer to the shop "object"
 * @param time_diff duration of the attendances
 * @return 0 on success, otherwise means errors occurred
 */
int countdown_end(shop_t * shop, time_t time_diff);

/*
 * @brief Updates the balcao statistics such as average attendance time, etc...
 * @param shop pointer to the shop "object"
 * @param time_diff duration of the balcao opening
 * @return 0 on success, otherwise means errors occurred
 */
int update_statistics(shop_t *shop, time_t time_diff);

/*
 * @brief Displays the balcao data collected overtime
 * @param shop pointer to the shop "object"
 */
void display_balcao_statistics(shop_t *shop);

/*
 * @brief Displays the shop data collected overtime
 * @param shop pointer to the shop "object"
 * @param shmem name of the shared memory
 */
void display_loja_statistics(shop_t *shop, char* shmem);

/*
 * @brief Increments the balcao attendance when a new client is added
 * @param shop pointer to the shop "object"
 * @return 0 on success, otherwise means errors occurred
 */
int inc_balcao_attendance(shop_t *shop);

/*
 * @brief Decrements the balcao attendance when a client's attendance has finished
 * @return 0 on success, otherwise means errors occurred
 */
int dec_balcao_attendance(shop_t *shop);

/*
 * @brief Terminates the balcao and the shop if it is the last balcao active
 * @param shmem name of the shared memory
 * @param shop pointer to the shop "object"
 * @return 0 on success, otherwise means errors occurred
 */
int terminate_balcao(char* shmem, shop_t *shop);

#endif
