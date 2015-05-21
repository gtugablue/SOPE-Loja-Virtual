#include "ger_cl.h"
#include "log.h"

int debug;
char *own_name;

int main(int argc, char **argv)
{
	////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////// Check if arguments are valid and initialize variables /////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////

	if(debug) printf("\t==> DEBUG[%s]: Starting ger_cl\n", argv[0]);

	own_name = argv[0];
	int i;
	int non_optional = 0;
	char *non_opt_args[argc-1];
	debug = 0;

	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-db") == 0)  /* Process optional arguments. */
			debug = 1;
		else
			non_opt_args[non_optional++] = argv[i];
	}

	if(debug) printf("\t==> DEBUG[%s]: Verifying arguments\n", argv[0]);

	if(non_optional != 2)
	{
		printf("\n\t%s: Invalid number of arguments.\n\tMust be: %s <shared_memory_name> <number_of_clients>\n\n", argv[0], argv[0]);
		return 1;
	}

	long num_clients;

	if(parse_long(&num_clients, non_opt_args[1], 10) != 0)
	{
		printf("\n\t%s - ERROR: Invalid number of clients.\n\tThat value is not a number\n\n", argv[0]);
		return 1;
	}

	shop_t *shop = NULL;
	int shm_key = 0;
	if(retrieve_shop(shop, &shm_key, non_opt_args[0]) != 0)
	{
		printf("\n\t%s - ERROR: Unable to open the specified memory region\n\n", argv[0]);
		return 1;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////// Initialize child processes /////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////

	if(debug) printf("\t==> DEBUG[%s]: Creating child processes\n", argv[0]);

	int temp = 0;
	int fork_return = 0;

	while(temp++ < num_clients)		// Cycle to create specified number of clients
	{
		fork_return = fork();

		if(fork_return == 0)
			break;
		else if(fork_return < 0)
		{
			printf("\n\t%s - INTERNAL ERROR: Unable to create processes using fork\n\n", argv[0]);
			return 2;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////// Give each process it's task (parent waits, child becomes client) ///////////////
	////////////////////////////////////////////////////////////////////////////////////////////////

	if(fork_return == 0)	// Child-processes - each represents a client who chooses a "balcao" to be attended at
		return child_action(non_opt_args[0], shm_key);
	else					// Parent-process - must wait for each child to finnish before terminating
		return parent_action();

	return 0;
}

int parent_action()
{
	if(debug) printf("\t==> DEBUG[%s]: Parent working\n", own_name);

	int child_status = 0;

	while(wait(&child_status) >= 0)
	{
		if(child_status != 0)
		{
			printf("\n\t%s - ERROR: Unexpected child process return (value %d)\n\n", own_name, child_status);
			return child_status;
		}
	}

	return 0;
}

int child_action(char *shname, int key)
{
	shop_t *shop = child_remap_shmem(shname, key);
	if(shop == NULL) return 1;

	if(debug) printf("\t==> DEBUG[%s]: Child working\n", own_name);

	char* fifo_pathname = get_fifo_pathname(getpid());
	int fifo_pathname_len = strlen(fifo_pathname);
	fifo_pathname[fifo_pathname_len++] = '\0';

	if(debug) printf("\t==> DEBUG[%s]: Constructed fifo_pathname \"%s\"\n", own_name, fifo_pathname);

	if(mkfifo(fifo_pathname, CL_FIFO_MODE) != 0)
	{
		printf("Error: unable to create client FIFO.\n");
		return 1;
	}

	int fifo_read = open(fifo_pathname, O_RDONLY | O_NONBLOCK);
	int fifo_write = open(fifo_pathname, O_WRONLY);
	if(debug) printf("\t==> DEBUG[%s]: Created fifo write(%d) read(%d)\n", own_name, fifo_write, fifo_read);
	if(fifo_read < 0 || fifo_write < 0)
	{
		printf("Error: unable to open client FIFO for reading.\n");
		return 1;
	}
	if(debug) printf("\t==> DEBUG[%s]: FIFO opening successful\n", own_name);

	char balcao_message[MAX_FIFO_NAME_LEN];

	int min_occup = INT_MAX;
	size_t min_occup_index = -1;
	size_t i;
	int balcao_fifo_fd = -1;
	if(debug) printf("\t==> DEBUG[%s]: First loja lock\n", own_name);

	if(attempt_mutex_lock(&(shop->loja_mutex), "loja") != 0) return 1;

	if(debug) printf("\t==> DEBUG[%s]: Accessing loja\n", own_name);
	size_t num_balcoes = shop->num_balcoes;

	if(num_balcoes > 0)
	{
		if(debug) printf("\t==> DEBUG[%s]: Choosing balcao\n", own_name);
		for(i = 0; i < num_balcoes; i++)
		{
			printf("balcao[%d][%d], %d clientes\n", (int)i, shop->balcoes[i].num, shop->balcoes[i].clientes_em_atendimento);
			if((shop->balcoes[i].duracao == -1) && (shop->balcoes[i].clientes_em_atendimento < min_occup))
			{
				min_occup = shop->balcoes[i].clientes_em_atendimento;
				min_occup_index = i;
			}
		}

		if(debug) printf("\t==> DEBUG[%s]: Chosen balcao %d\n", own_name, shop->balcoes[min_occup_index].num);

		if(attempt_mutex_lock(&(shop->balcoes[min_occup_index].balcao_mutex), "balcao") != 0)
		{
			attempt_mutex_unlock(&(shop->loja_mutex), "loja");
			return 1;
		}

		if(attempt_mutex_unlock(&(shop->loja_mutex), "loja") != 0) return 1;

		if(debug) printf("\t==> DEBUG[%s]: Writing to balcao\n", own_name);
		balcao_fifo_fd = open(shop->balcoes[min_occup_index].fifo_name, O_WRONLY);

		if(balcao_fifo_fd < 0)
		{
			printf("Error: unable to open \"balcao\" FIFO.\n");

			attempt_mutex_unlock(&(shop->balcoes[min_occup_index].balcao_mutex), "balcao");
			attempt_mutex_unlock(&(shop->loja_mutex), "loja");
			return 1;
		}

		if(write(balcao_fifo_fd, fifo_pathname, fifo_pathname_len) != fifo_pathname_len)
		{
			printf("Error: problems writing to \"balcao\" FIFO.\n");

			attempt_mutex_unlock(&(shop->balcoes[min_occup_index].balcao_mutex), "balcao");
			attempt_mutex_unlock(&(shop->loja_mutex), "balcao");

			return 1;
		}

		if (write_log_entry(shname, CLIENT, min_occup_index + 1, "pede_atendimento", shop->balcoes[min_occup_index].fifo_name))
		{
			printf("Warning: could not write to logfile.\n");
		}

		if(debug) printf("\t==> DEBUG[%s]: Read from balcao\n", own_name);
		while(1)
		{
			read(fifo_read, balcao_message, MAX_FIFO_NAME_LEN);

			if(strcmp(balcao_message, ATTEND_END_MESSAGE) == 0)
				break;
		}

		if(attempt_mutex_unlock(&(shop->balcoes[min_occup_index].balcao_mutex), "balcao") != 0) return 1;
	}
	else
	{
		if(attempt_mutex_unlock(&(shop->loja_mutex), "loja") != 0)
			printf("Error: unable to unlock \"loja\" mutex.\n");

		printf("Error: the specified shop has no valid balcons.\n");
		return 1;
	}

	if(debug) printf("\t==> DEBUG[%s]: Finishing well\n", own_name);

	close(fifo_read);
	close(fifo_write);
	if(unlink(fifo_pathname) != 0) {
		printf("Error: unable to unlink client FIFO.\n");
		return 1;
	}
	free(fifo_pathname);
	return 0;
}

int retrieve_shop(shop_t *shop, int *key, char *shm_name)
{
	if((*key = shm_open(shm_name, O_RDWR, SHARED_MEM_MODE)) < 0)
		return 1;

	shop = (shop_t *) mmap(0,SHARED_MEM_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,*key,0);
	if(shop == MAP_FAILED)
		return 1;

	return 0;
}

char *get_fifo_pathname(int pid)
{
	char* pid_str = malloc(MAX_FIFO_NAME_LEN);
	sprintf(pid_str, "%d", pid);

	char *fifo_pathname = malloc(MAX_FIFO_NAME_LEN);
	strcpy(fifo_pathname, CL_FIFO_NAME);
	strcpy(fifo_pathname + strlen(CL_FIFO_NAME), pid_str);		// fifo_pathname now has the complete path for the fifo

	free(pid_str);

	return fifo_pathname;
}

shop_t *child_remap_shmem(char* shmem_name, int key)
{
	shop_t *shop = NULL;
	shop = (shop_t *) mmap(0,SHARED_MEM_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,key,0);
	if(shop == MAP_FAILED)
	{
		printf("Error: unable to remap shmem on child process");
		return NULL;
	}
	return shop;
}

int attempt_mutex_lock(pthread_mutex_t *mutex, char *name)
{
	if(pthread_mutex_lock(mutex) != 0)
	{
		printf("Error: unable to lock \"%s\" mutex.\n", name);
		return 1;
	}
	return 0;
}

int attempt_mutex_unlock(pthread_mutex_t *mutex, char *name)
{
	if(pthread_mutex_unlock(mutex) != 0)
	{
		printf("Error: unable to unlock \"%s\" mutex.\n", name);
		return 1;
	}
	return 0;
}


