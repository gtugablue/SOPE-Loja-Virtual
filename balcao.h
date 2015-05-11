#include <stdio.h>
#include "utils.h"
#include <sys/shm.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <unistd.h>

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

} Balcao

typedef struct Loja_t
{
   long data_abertura;
   int num_balcoes;
   // TODO Syncronizing variables
   // ...

   Balcao *balcoes;

} Loja

int main(int argc, char *argv[]);
int create_shared_memory(int *shm_id);
