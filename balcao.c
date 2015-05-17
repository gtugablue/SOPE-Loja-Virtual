#include "balcao.h"

int main(int argc, char *argv[])
{
	if(argc != 3)
	{
		printf("\n\t%s: Invalid number of arguments.\n\tMust be: %s <shared_memory_name> <opening_time>\n\n", argv[0], argv[0]);
		return 1;
	}

	int opening_duration = 0;
	if(parse_int(&opening_duration, argv[2], 10) != 0)
	{
		printf("\n\t%s - ERROR: Invalid opening time. Must be a number\n\n", argv[0]);
		return 1;
	}

	int shm_id;
	shop_t *shop;
	shop = (shop_t *)create_shared_memory(argv[1], &shm_id, SHARED_MEM_SIZE);
	if (shop == NULL) return 1;

	balcao_t balcao = join_shmemory(shop);

	if(balcao.num == -1) return 1;

	// TODO Atender clientes

	return terminate_balcao(argv[1], shop);
}

shop_t *create_shared_memory(const char *name, int *shm_id, long size)
{
	if((*shm_id = shm_open(name, O_RDWR, 0600)) < 0)
	{
		if((*shm_id = shm_open(name, O_CREAT | O_EXCL | O_RDWR, 0600)) < 0)
		{
			printf("Error: couldn't create/open shared memory.\n");
			return NULL;
		}
		if (ftruncate(*shm_id, size) == -1)
		{
			printf("Error: couldn't allocate space in the shared memory.\n");
			return NULL;
		}

		time_t time_open = time(NULL);

		shop_t shop;
		shop.opening_time = time_open;
		pthread_mutex_t mt = PTHREAD_MUTEX_INITIALIZER;	// had to do this to solve compilation error
		shop.loja_mutex = mt;
		shop.num_balcoes = 0;
		//TODO tabela balcões

		shop_t *shmem;
		shmem = (shop_t *) mmap(0,size,PROT_READ|PROT_WRITE,MAP_SHARED,*shm_id,0);
		*shmem = shop;
		return shmem;
	}

	if (ftruncate(*shm_id, size) == -1)
	{
		printf("Error: couldn't allocate space in the shared memory.\n");
		return NULL;
	}

	shop_t *shmem;
	shmem = (shop_t *) mmap(0,size,PROT_READ|PROT_WRITE,MAP_SHARED,*shm_id,0);
	return shmem;
}

balcao_t join_shmemory(shop_t* shop)
{
	balcao_t thisBalcao; thisBalcao.num = -1;

	if(pthread_mutex_lock(&shop->loja_mutex) != 0)
	{
		printf("Error: unable to lock \"loja\" mutex.\n");
		return thisBalcao;
	}

	time_t curr_time = time(NULL);
	int pid = getpid();

	thisBalcao.num = shop->num_balcoes + 1;
	thisBalcao.abertura = curr_time;
	thisBalcao.duracao = (time_t)-1;
	// TODO criar nome do fifo - fb[pid]
	thisBalcao.clientes_em_atendimento = 0;
	thisBalcao.clientes_atendidos = 0;
	thisBalcao.atendimento_medio = 0;

	shop->num_balcoes++;
	//TODO adicionar objeto do balcao à loja

	pthread_mutex_unlock(&shop->loja_mutex);

	return thisBalcao;
}

int terminate_balcao(char* shmem, shop_t *shop)
{
	pthred_mutex_lock(&shop->loja_mutex);

	// TODO terminate balcao FIFO

	if (shm_unlink(shmem) == -1)
	{
		printf("Error: shared memory wasn't properly cleaned.\n");
		return 1;
	}

	return 0;
}

