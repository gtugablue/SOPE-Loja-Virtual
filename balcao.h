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

#define SHARED_MEM_SIZE	(1024 * 1024)
#define ATTEND_END_MESSAGE "fim_atendimento"

#define MAX_FIFO_NAME_LEN 20
#define BALCAO_FIFO_MODE 0600

#define MAX_NUM_BALCOES 30

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
	long atendimento_medio;
} balcao_t;

typedef struct
{
	// Synchronizing variables
	pthread_mutex_t loja_mutex;

	// Data variables
	time_t opening_time;
	int num_balcoes;
	balcao_t balcoes[MAX_NUM_BALCOES];
} shop_t;

typedef struct {
	int *curr_count;
	int *fifo_write_fd;
	shop_t *shop;
} counter_thr_info;

typedef struct {
	int duration;
	char *cl_fifo;
} attend_thr_info;

int main(int argc, char *argv[]);
shop_t *create_shared_memory(const char *name, int *shm_id, long size);
balcao_t join_shmemory(shop_t* shop);
int terminate_balcao(char* shmem, shop_t *shop);
void *timer_countdown(void *arg);
void *attend_client(void *arg);
int initialize_log(const char *sh_name);

#endif
