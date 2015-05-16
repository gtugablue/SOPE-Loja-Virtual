#ifndef SHARED_MEM_H
#define SHARED_MEM_H

typedef struct
{
	int num;
	time_t starting_time;
} balcao_t;

typedef struct
{
	// TODO primitivas
	time_t opening_time;
	int num_balcao;
	balcao_t *balcoes;
} shop_info_t;

#endif
