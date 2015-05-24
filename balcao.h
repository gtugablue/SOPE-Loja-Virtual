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

int init(const char *shname, shop_t **shop, int *shm_id);
shop_t *create_shared_memory(const char *name, int *shm_id);
void initialize_shop_st(shop_t *shop);
int join_shmemory(const char *shname, shop_t **shop);

void *timer_countdown(void *arg);
void *attend_client(void *arg);
int read_fifo(int fifo_fd, char** non_opt_args, shop_t *shop);

int initialize_log(const char *sh_name);
int countdown_end(shop_t * shop, time_t time_diff);
int update_statistics(shop_t *shop, time_t time_diff);
void display_balcao_statistics(shop_t *shop);
void display_loja_statistics(shop_t *shop, char* shmem);
int terminate_balcao(char* shmem, shop_t *shop);

#endif
