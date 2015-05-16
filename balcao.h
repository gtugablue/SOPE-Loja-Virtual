#ifndef BALCAO_H
#define BALCAO_H

#include <stdio.h>
#include "utils.h"
#include <sys/shm.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define SHARED_MEM_SIZE	(1024 * 1024)

typedef struct Balcao_t
{
	int num;
	long abertura;
	long duracao;
	char* fifo_name;
	int clientes_em_atendimento;
	int clientes_atendidos;
	long atendimento_medio;
} balcao_t;

int main(int argc, char *argv[]);
int create_shared_memory(const char *name, int *shm_id, long size);

#endif
