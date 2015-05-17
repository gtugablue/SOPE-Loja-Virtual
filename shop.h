#ifndef SHARED_MEM_H
#define SHARED_MEM_H

#include "balcao.h"

typedef struct
{
	// Synchronizing variables
	pthread_mutex_t loja_mutex;

	// Data variables
	time_t opening_time;
	int num_balcoes;
	balcao_t *balcoes;
} shop_t;

#endif
