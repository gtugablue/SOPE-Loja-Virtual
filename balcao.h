#ifndef BALCAO_H
#define BALCAO_H

#include <pthread.h>

#define MAX_FIFO_NAME_LEN 14
#define BALCAO_FIFO_MODE 0600

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
#include "utils.h"
#include "shop.h"
#include "ger_cl.h"

#define SHARED_MEM_SIZE	(1024 * 1024)

int main(int argc, char *argv[]);
shop_t *create_shared_memory(const char *name, int *shm_id, long size);
balcao_t join_shmemory(shop_t* shop);
int terminate_balcao(char* shmem, shop_t *shop);
void *timer_countdown(void *arg);
int initialize_log(const char *sh_name);

#endif
