#include "balcao.h"
#include "log.h"

int ownIndex;

char *own_name;
int debug;

int main(int argc, char *argv[])
{
	///////////////////////////////////////////////////////
	/////////////// Argument verification /////////////////
	///////////////////////////////////////////////////////

	own_name = argv[0];

	int i;
	int non_optional = 0;
	char *non_opt_args[argc-1];

	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-db") == 0)  /* Process optional arguments. */
			debug = 1;
		else
			non_opt_args[non_optional++] = argv[i];
	}

	if(debug) printf("\t==> DEBUG[%s - %d]: Starting balcao\n", own_name, getpid());
	if(debug) printf("\t==> DEBUG[%s - %d]: Verifying arguments\n", own_name, getpid());

	if(non_optional != 2)
	{
		printf("\n\t%s: Invalid number of arguments.\n\tMust be: %s <shared_memory_name> <opening_time>\n\t         %s -c <sh-mem name to clean>\n\t \"-db\" to debug\n\n", argv[0], argv[0], argv[0]);
		return 1;
	}

	if(strcmp("-c", non_opt_args[0]) == 0) {
		shm_unlink(non_opt_args[1]);
		printf("\n\t%s: Cleaned shared memory \"%s\"\n\n", argv[0], non_opt_args[1]);
		return 0;
	}

	int opening_duration = 0;

	if(parse_int(&opening_duration, non_opt_args[1], 10) != 0 || opening_duration <= 0)
	{
		printf("\n\t%s - ERROR: Invalid opening time. Must be a number greater than zero\n\n", argv[0]);
		return 1;
	}

	int shm_id;
	shop_t *shop;
	if (init(non_opt_args[0], &shop, &shm_id)) return 1;

	///////////////////////////////////////////////////////
	//////////////// Preparing threads ////////////////////
	///////////////////////////////////////////////////////

	if(debug) printf("\t==> DEBUG[%s - %d]: Preparing for threads\n", own_name, getpid());

	pthread_t counterThread;
	int curr_count = opening_duration;

	char *fifo_name = shop->balcoes[ownIndex].fifo_name;
	printf("fifo_name: %s\n", fifo_name);
	char path[strlen(FIFO_DIR) + strlen(fifo_name) + 1];
	strcpy(path, FIFO_DIR);
	strcat(path, fifo_name);
	counter_thr_info info;
	info.curr_count = &curr_count;
	info.path = path;
	info.shop = shop;
	if(debug) printf("\t==> DEBUG[%s - %d]: Launching countdown thread\n", own_name, getpid());
	pthread_create(&counterThread, NULL, timer_countdown, &info);

	if(debug) printf("\t==> DEBUG[%s - %d]: Opening FIFO %s\n", own_name, getpid(), path);
	int fifo_fd = open(path, O_RDONLY);
	if(fifo_fd <= 0)
	{
		printf("Error: could not open fifo.\n");
		return 1;
	}


	if(debug) printf("\t==> DEBUG[%s - %d]: Starting read cycle\n", own_name, getpid());

	if(read_fifo(fifo_fd, non_opt_args, shop)) return 1;

	if(debug) printf("\t==> DEBUG[%s - %d]: Terminating balcao\n", own_name, getpid());

	return terminate_balcao(non_opt_args[0], shop);
}

int init(const char *shname, shop_t **shop, int *shm_id)
{
	if(debug) printf("\t==> DEBUG[%s - %d]: Initializing shop and balcao\n", own_name, getpid());

	*shop = create_shared_memory(shname, shm_id);
	if (*shop == NULL) return 1;

	if (join_shmemory(shname, shop)) return 1;

	return 0;
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

		shop_t shop;
		initialize_shop_st(&shop);

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

int join_shmemory(const char *shname, shop_t **shop)
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
		return 1;
	}

	thisBalcao.clientes_em_atendimento = 0;
	thisBalcao.clientes_atendidos = 0;
	thisBalcao.atendimento_medio = 0;

	if(attempt_mutex_lock(&((*shop)->loja_mutex), "loja", debug) != 0) return 1;

	int num_balcoes = (*shop)->num_balcoes;
	thisBalcao.num = num_balcoes + 1;
	(*shop)->balcoes[num_balcoes] = thisBalcao;
	(*shop)->num_balcoes++;
	(*shop)->num_balcoes_abertos++;

	pthread_mutexattr_t mattr;
	pthread_mutexattr_init(&mattr); /* inicializa variável de atributos */
	pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&(*shop)->balcoes[num_balcoes].balcao_mutex, &mattr);

	attempt_mutex_unlock(&((*shop)->loja_mutex), "loja", debug);

	ownIndex = num_balcoes;

	if (write_log_entry(shname, BALCAO, thisBalcao.num, "cria_linh_mempart", thisBalcao.fifo_name))
	{
		printf("Error writting to log.\n");
		return 1;
	}

	return 0;
}

int terminate_balcao(char* shmem, shop_t *shop)
{
	attempt_mutex_lock(&(shop->loja_mutex), "loja", debug);
	--shop->num_balcoes_abertos;

	if(shop->num_balcoes_abertos == 0)	// this is the last balcao active
	{
		attempt_mutex_unlock(&(shop->loja_mutex), "loja", debug);
		attempt_mutex_destroy(&(shop->loja_mutex), "loja", debug);

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

		printf("\n\t==> Shared memory cleaned\n");
	}
	else
	{
		attempt_mutex_unlock(&(shop->loja_mutex), "loja", debug);
	}

	printf("\t==> Balcao terminating\n\n");

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

	if(debug) printf("\t==> DEBUG[%s - %d]: Countdown ended\n", own_name, getpid());

	info->shop->balcoes[ownIndex].duracao = time(NULL) - info->shop->balcoes[ownIndex].abertura; // TODO mutex
	close(fifo_write);
	return NULL;
}

void *attend_client(void *arg)
{
	if(debug) printf("\t==> DEBUG[%s - %d]: Attending client with FIFO %s\n", own_name, getpid(), ((attend_thr_info*)arg)->cl_fifo);

	char *cl_fifo = ((attend_thr_info*)arg)->cl_fifo;
	int duration = ((attend_thr_info*)arg)->duration;
	sleep(duration);

	if(debug) printf("\t==> DEBUG[%s - %d]: Opening FIFO %s\n", own_name, getpid(), cl_fifo);
	int cl_fifo_fd = open(cl_fifo, O_WRONLY);
	if(debug) printf("\t==> DEBUG[%s - %d]: Opened FIFO %s\n", own_name, getpid(), cl_fifo);

	if(cl_fifo_fd > 0)
	{
		if (write_log_entry(((attend_thr_info*)arg)->shname, BALCAO, 1, "fim_atend_cli", cl_fifo))
		{
			return NULL;
		}

		if(debug) printf("\t==> DEBUG[%s - %d]: Writing \"%s\" to \"%s\"\n", own_name, getpid(), ATTEND_END_MESSAGE, cl_fifo);

		int r;
		if((r = write(cl_fifo_fd, ATTEND_END_MESSAGE, strlen(ATTEND_END_MESSAGE) + 1)) != strlen(ATTEND_END_MESSAGE) + 1)
			printf("Error: different bytes written(%d)\n", r);
		close(cl_fifo_fd);
	}
	else
		printf("\t==> ERROR: Unable to open client fifo [%s]\n", cl_fifo);

	free(((attend_thr_info*)arg)->cl_fifo);
	free((attend_thr_info*)arg);
	return NULL;
}

int read_fifo(int fifo_fd, char** non_opt_args, shop_t *shop)
{
	// Initialize auxiliar variables
	int str_size = 0;
	do
	{
		char message[MAX_FIFO_NAME_LEN+1];
		char *thr_arg;
		attend_thr_info *cl_info;

		// Start reading
		printf("Blocking on read...\n");

		str_size = read(fifo_fd, message, MAX_FIFO_NAME_LEN);
		if(str_size <= 0) return 0;
		printf("\t==> Client FIFO read: %s, %d chars read.\n", message, str_size);

		pthread_t attendThread;

		thr_arg = malloc(MAX_FIFO_NAME_LEN+1); // TODO check for errors and free
		message[str_size] = '\0';
		strcpy(thr_arg, message);

		cl_info = malloc(sizeof(attend_thr_info));
		cl_info->cl_fifo = thr_arg;
		cl_info->shname = non_opt_args[0];
		// TODO mutex?
		int duration = shop->balcoes[ownIndex].clientes_em_atendimento + 1;
		if(duration > 10) duration = 10;
		cl_info->duration = duration;
		pthread_create(&attendThread, NULL, attend_client, cl_info);

		if (write_log_entry(non_opt_args[0], BALCAO, 1, "inicia_atend_cli", cl_info->cl_fifo))
		{
			printf("Error writting to log.\n");
			return 1;
		}
	}
	while (str_size != 0);
	return 0;
}

void initialize_shop_st(shop_t *shop)
{
	shop->opening_time = time(NULL);
	pthread_mutex_t mt = PTHREAD_MUTEX_INITIALIZER;	// had to do this to solve compilation error

	shop->loja_mutex = mt;
	shop->num_balcoes = 0;

	pthread_mutexattr_t mattr;
	pthread_mutexattr_init(&mattr); /* inicializa variável de atributos */
	pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&shop->loja_mutex, &mattr);
}
