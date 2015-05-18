#include "balcao.h"
#include "log.h"

int ownIndex;

int main(int argc, char *argv[])
{
	///////////////////////////////////////////////////////
	/////////////// Argument verification /////////////////
	///////////////////////////////////////////////////////

	if(argc != 3)
	{
		printf("\n\t%s: Invalid number of arguments.\n\tMust be: %s <shared_memory_name> <opening_time>\n\n", argv[0], argv[0]);
		return 1;
	}

	int opening_duration = 0;

	if(parse_int(&opening_duration, argv[2], 10) != 0 || opening_duration <= 0)
	{
		printf("\n\t%s - ERROR: Invalid opening time. Must be a number greater than zero\n\n", argv[0]);
		return 1;
	}

	if (initialize_log(argv[1]))
	{
		printf("Error initializing log.\n");
		return 1;
	}

	///////////////////////////////////////////////////////
	/////////////// Shop and balcao init //////////////////
	///////////////////////////////////////////////////////

	int shm_id;
	shop_t *shop = NULL;
	shop = (shop_t *)create_shared_memory(argv[1], &shm_id, SHARED_MEM_SIZE);
	if (shop == NULL) return 1;

	balcao_t balcao = join_shmemory(shop);

	if(balcao.num == -1) return 1;

	pthread_t counterThread, attendThread;
	int curr_count = opening_duration;

	char message[MAX_FIFO_NAME_LEN+1];
	char *thr_arg;
	int str_size = 0;

	char *fifo_name = shop->balcoes[ownIndex].fifo_name;

	int fifo_fd = open(fifo_name, O_RDONLY | O_NONBLOCK);
	int fifo_write = open(fifo_name, O_WRONLY);
	if((fifo_fd <= 0) | (fifo_write <= 0))
	{
		printf("Error: could not open fifo.\n");
		return 1;
	}

	///////////////////////////////////////////////////////
	///////////////// Thread deployment ///////////////////
	///////////////////////////////////////////////////////

	counter_thr_info info;
	info.curr_count = &curr_count;
	info.fifo_write_fd = &fifo_write;
	info.shop = shop;

	pthread_create(&counterThread, NULL, timer_countdown, &info);
	while(curr_count > 0)
	{
		str_size = read(fifo_fd, message, MAX_FIFO_NAME_LEN);

		thr_arg = malloc(MAX_FIFO_NAME_LEN+1);
		message[str_size] = '\0';
		strcpy(thr_arg, message);
		attend_thr_info cl_info;
		cl_info.cl_fifo = thr_arg;
		int duration = shop->balcoes[ownIndex].clientes_em_atendimento + 1;
		if(duration > 10) duration = 10;
		cl_info.duration = duration;

		pthread_create(&attendThread, NULL, attend_client, &cl_info);
	}

	//return 0;
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

	time_t curr_time = time(NULL);
	int pid = getpid();

	thisBalcao.abertura = curr_time;
	thisBalcao.duracao = (time_t)-1;
	sprintf(thisBalcao.fifo_name, "/tmp/fb_%d", pid);

	if(mkfifo(thisBalcao.fifo_name, BALCAO_FIFO_MODE) != 0)
	{
		printf("Error: unable to create FIFO %s\n", thisBalcao.fifo_name);
		return thisBalcao;
	}

	thisBalcao.clientes_em_atendimento = 0;
	thisBalcao.clientes_atendidos = 0;
	thisBalcao.atendimento_medio = 0;

	if(pthread_mutex_lock(&shop->loja_mutex) != 0)
	{
		printf("Error: unable to lock \"loja\" mutex.\n");
		return thisBalcao;
	}

	int num_balcoes = shop->num_balcoes;
	thisBalcao.num = num_balcoes + 1;
	ownIndex = num_balcoes;
	shop->balcoes[num_balcoes] = thisBalcao;
	shop->num_balcoes++;

	pthread_mutex_unlock(&shop->loja_mutex);

	return thisBalcao;
}

int terminate_balcao(char* shmem, shop_t *shop)
{
	int last = 0;
	int i = 0;
	for(; i < shop->num_balcoes; i++)
	{
		if((int)shop->balcoes[i].duracao == -1)
		{
			last = 1;
			break;
		}
	}

	if(last == 0)	// this is the last balcao active
	{
		pthread_mutex_lock(&shop->loja_mutex);
		pthread_mutex_unlock(&shop->loja_mutex);	// to ensure no process is using it
		pthread_mutex_destroy(&shop->loja_mutex);

		if(unlink(shop->balcoes[ownIndex].fifo_name) != 0)
		{
			printf("Error: unable to unlink fifo %s\n", shop->balcoes[ownIndex].fifo_name);
			return 1;
		}

		if (shm_unlink(shmem) == -1)
		{
			printf("Error: shared memory wasn't properly cleaned.\n");
			return 1;
		}

		printf("\nShared memory cleaned\n\n");
	}



	return 0;
}

void *timer_countdown(void *arg)
{
	counter_thr_info *info = (counter_thr_info*)arg;
	int *count = info->curr_count;
	time_t start_time = time(NULL);
	time_t curr_time;
	while(1)
	{
		curr_time = time(NULL);
		if(curr_time-start_time >= *count)
		{
			*count = 0;
			break;
		}
		sleep(0.1);
	}

	info->shop->balcoes[ownIndex].duracao = time(NULL) - info->shop->balcoes[ownIndex].abertura;
	close(*info->fifo_write_fd);
	return NULL;
}

void *attend_client(void *arg)
{
	char *cl_fifo = ((attend_thr_info*)arg)->cl_fifo;
	int duration = ((attend_thr_info*)arg)->duration;

	sleep(duration);

	int cl_fifo_fd = open(cl_fifo, O_WRONLY);

	if(cl_fifo_fd > 0)
	{
		char *message = ATTEND_END_MESSAGE;
		write(cl_fifo_fd, message, strlen(message));
		close(cl_fifo_fd);
	}

	//free((attend_thr_info*)arg);
	return NULL;
}
