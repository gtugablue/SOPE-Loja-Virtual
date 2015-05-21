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

	if(strcmp("-c", argv[1]) == 0) {
		shm_unlink(argv[2]);
		return 0;
	}

	int opening_duration = 0;

	if(parse_int(&opening_duration, argv[2], 10) != 0 || opening_duration <= 0)
	{
		printf("\n\t%s - ERROR: Invalid opening time. Must be a number greater than zero\n\n", argv[0]);
		return 1;
	}

	///////////////////////////////////////////////////////
	/////////////// Shop and balcao init //////////////////
	///////////////////////////////////////////////////////

	int shm_id = -1;
	shop_t *shop = NULL;
	shop = (shop_t *)create_shared_memory(argv[1], &shm_id);
	if (shop == NULL) return 1;

	balcao_t balcao = join_shmemory(argv[1], shop);

	if(balcao.num == -1) return 1;

	pthread_t counterThread, attendThread;
	int curr_count = opening_duration;

	char message[MAX_FIFO_NAME_LEN+1];
	char *thr_arg;
	int str_size = 0;

	char *fifo_name = shop->balcoes[ownIndex].fifo_name;
	char path[strlen(FIFO_DIR) + strlen(fifo_name) + 1];
	strcpy(path, FIFO_DIR);
	strcat(path, fifo_name);
	int fifo_fd = open(path, O_RDONLY | O_NONBLOCK);
	int fifo_write = open(path, O_WRONLY);
	if((fifo_fd <= 0) | (fifo_write <= 0))
	{
		printf("Error: could not open fifo.\n");
		return 1;
	}

	///////////////////////////////////////////////////////
	///////////////// Thread deployment ///////////////////
	///////////////////////////////////////////////////////

	write(fifo_write, "hello", strlen("hello"));

	counter_thr_info info;
	info.curr_count = &curr_count;
	info.fifo_write_fd = &fifo_write;
	info.shop = shop;

	printf("\t==> Starting while\n");
	pthread_create(&counterThread, NULL, timer_countdown, &info);
	while(curr_count > 0)
	{
		str_size = read(fifo_fd, message, MAX_FIFO_NAME_LEN);
		if(str_size <= 0 || message[0] != '\\') continue;
		printf("\t==> Read message %s\n", message);
		continue;

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

shop_t *create_shared_memory(const char *name, int *shm_id)
{
	if((*shm_id = shm_open(name, O_RDWR, SHARED_MEM_MODE)) < 0)
	{
		if((*shm_id = shm_open(name, O_CREAT | O_EXCL | O_RDWR, SHARED_MEM_MODE)) < 0)
		{
			printf("Error: couldn't create/open shared memory.\n");
			return NULL;
		}
		if (ftruncate(*shm_id, SHARED_MEM_SIZE) == -1)
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
		shmem = (shop_t *) mmap(0,SHARED_MEM_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED, *shm_id, 0);
		if(shmem == MAP_FAILED)
		{
			printf("Error mapping shared memory.\n");
			return NULL;
		}
		*shmem = shop;

		if (initialize_log(name))
		{
			printf("Error initializing log.\n");
			return NULL;
		}
		if (write_log_entry(name, BALCAO, 1, "inicia_mempart", "-"))
		{
			printf("Error writting to log.\n");
			return NULL;
		}

		return shmem;
	}

	shop_t *shmem;
	shmem = (shop_t *) mmap(0,SHARED_MEM_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,*shm_id,0);
	return shmem;
}

balcao_t join_shmemory(const char *shname, shop_t* shop)
{
	balcao_t thisBalcao; thisBalcao.num = -1;

	time_t curr_time = time(NULL);
	int pid = getpid();

	thisBalcao.abertura = curr_time;
	thisBalcao.duracao = (time_t)-1;
	pthread_mutex_t mt = PTHREAD_MUTEX_INITIALIZER;
	thisBalcao.balcao_mutex = mt;
	sprintf(thisBalcao.fifo_name, "fb_%d", pid);
	char path[strlen(thisBalcao.fifo_name) + 1 + strlen(FIFO_DIR)];
	strcpy(path, FIFO_DIR);
	strcat(path, thisBalcao.fifo_name);
	//thisBalcao.fifo_name = path;
	//strcpy(thisBalcao.fifo_name, path);

	printf("==> MyFifo: %s\n", thisBalcao.fifo_name);

	if(mkfifo(path, BALCAO_FIFO_MODE) != 0)
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
	shop->balcoes[num_balcoes] = thisBalcao;
	shop->num_balcoes++;
	shop->num_balcoes_abertos++;

	pthread_mutex_unlock(&shop->loja_mutex);

	ownIndex = num_balcoes;

	if (write_log_entry(shname, BALCAO, thisBalcao.num, "cria_linh_mempart", thisBalcao.fifo_name))
	{
		printf("Error writting to log.\n");
		return thisBalcao;
	}

	return thisBalcao;
}

int terminate_balcao(char* shmem, shop_t *shop)
{
	pthread_mutex_lock(&shop->loja_mutex);
	--shop->num_balcoes_abertos;

	if(shop->num_balcoes_abertos == 0)	// this is the last balcao active
	{
		pthread_mutex_unlock(&shop->loja_mutex);
		pthread_mutex_destroy(&shop->loja_mutex);

		char path[strlen(FIFO_DIR) + strlen(shop->balcoes[ownIndex].fifo_name) + 1];
		strcpy(path, FIFO_DIR);
		strcat(path, shop->balcoes[ownIndex].fifo_name);

		if(unlink(path) != 0)
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
	else
	{
		pthread_mutex_unlock(&shop->loja_mutex);
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
	printf("Cl fifo [%s]\n", cl_fifo);
	int duration = ((attend_thr_info*)arg)->duration;

	printf("\t==> Attending client %s for %d seconds\n", cl_fifo, duration);

	sleep(duration);

	int cl_fifo_fd = open(cl_fifo, O_WRONLY);
	printf("\t==> FIFO fd [%d]\n", cl_fifo_fd);

	if(cl_fifo_fd > 0)
	{
		char *message = ATTEND_END_MESSAGE;
		int r;
		if((r = write(cl_fifo_fd, message, strlen(message))) != strlen(message))
			printf("Error: different bytes written(%d)\n", r);
		close(cl_fifo_fd);
	}

	//free((attend_thr_info*)arg);
	return NULL;
}
