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

	counter_thr_info info;
	info.curr_count = &curr_count;
	info.path = path;
	info.shop = shop;
	pthread_create(&counterThread, NULL, timer_countdown, &info);
	int fifo_fd = open(path, O_RDONLY);
	if(fifo_fd <= 0)
	{
		printf("Error: could not open fifo.\n");
		return 1;
	}

	attend_thr_info *cl_info;

	while(curr_count > 0)
	{
		str_size = read(fifo_fd, message, MAX_FIFO_NAME_LEN); // TODO read until '\0'
		if(str_size <= 0) continue;
		printf("Message received: %s\n", message);

		thr_arg = malloc(MAX_FIFO_NAME_LEN+1); // TODO check for errors and free
		message[str_size] = '\0';
		strcpy(thr_arg, message);

		cl_info = malloc(sizeof(attend_thr_info));
		cl_info->cl_fifo = thr_arg;
		cl_info->shname = argv[1];
		// TODO mutex?
		int duration = shop->balcoes[ownIndex].clientes_em_atendimento + 1;
		if(duration > 10) duration = 10;
		cl_info->duration = duration;
		pthread_create(&attendThread, NULL, attend_client, cl_info);

		char *cl_fifo_name = filenameFromPath(cl_info->cl_fifo);
		if (write_log_entry(argv[1], BALCAO, 1, "inicia_atend_cli", cl_fifo_name))
		{
			printf("Error writting to log.\n");
			return 1;
		}
		free(cl_fifo_name);
	}

	printf("==> Terminating balcao\n");

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

	if(attempt_mutex_lock(&(shop->loja_mutex), "loja") != 0) return thisBalcao;

	int num_balcoes = shop->num_balcoes;
	thisBalcao.num = num_balcoes + 1;
	shop->balcoes[num_balcoes] = thisBalcao;
	shop->num_balcoes++;
	shop->num_balcoes_abertos++;

	attempt_mutex_unlock(&(shop->loja_mutex), "loja");

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
	attempt_mutex_lock(&(shop->loja_mutex), "loja");
	--shop->num_balcoes_abertos;

	if(shop->num_balcoes_abertos == 0)	// this is the last balcao active
	{
		attempt_mutex_unlock(&(shop->loja_mutex), "loja");
		attempt_mutex_destroy(&(shop->loja_mutex), "loja");

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
		attempt_mutex_unlock(&(shop->loja_mutex), "loja");
	}

	return 0;
}

void *timer_countdown(void *arg)
{
	counter_thr_info *info = (counter_thr_info*)arg;
	int fifo_write = open(info->path, O_WRONLY);
	if (fifo_write <= 0)
	{
		printf("Error: could not open fifo.\n");
		return NULL;
	}

	int *count = info->curr_count;
	sleep(*count);
	*count = 0;

	printf("Countdown end\n");

	info->shop->balcoes[ownIndex].duracao = time(NULL) - info->shop->balcoes[ownIndex].abertura; // TODO mutex
	close(fifo_write);
	return NULL;
}

void *attend_client(void *arg)
{
	char *cl_fifo = ((attend_thr_info*)arg)->cl_fifo;
	int duration = ((attend_thr_info*)arg)->duration;
	sleep(duration);
	printf("Opening fifo %s\n", cl_fifo);
	int cl_fifo_fd = open(cl_fifo, O_WRONLY);
	printf("Opened fifo %s\n", cl_fifo);
	if(cl_fifo_fd > 0)
	{
		char *cl_fifo_name = filenameFromPath(cl_fifo);
		if (write_log_entry(((attend_thr_info*)arg)->shname, BALCAO, 1, "fim_atend_cli", cl_fifo_name))
		{
			return NULL;
		}
		free(cl_fifo_name);
		int r;
		printf("==> Writing [%s] to [%s]\n", ATTEND_END_MESSAGE, cl_fifo);
		if((r = write(cl_fifo_fd, ATTEND_END_MESSAGE, strlen(ATTEND_END_MESSAGE) + 1)) != strlen(ATTEND_END_MESSAGE) + 1)
			printf("Error: different bytes written(%d)\n", r);
		close(cl_fifo_fd);
	}
	else
		printf("==> Unable to open client fifo [%s]\n", cl_fifo);

	free(((attend_thr_info*)arg)->cl_fifo);
	free((attend_thr_info*)arg);
	return NULL;
}
