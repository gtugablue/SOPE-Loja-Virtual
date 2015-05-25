#include "ger_cl.h"
#include "log.h"

int debug;
char *own_name;

int main(int argc, char **argv)
{
	////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////// Check if arguments are valid and initialize variables /////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////

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

	if(debug) printf("\t==> DEBUG[%s - %d]: Starting ger_cl\n", argv[0], getpid());
	if(debug) printf("\t==> DEBUG[%s - %d]: Verifying arguments\n", argv[0], getpid());

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

	if(debug) printf("\t==> DEBUG[%s - %d]: Creating child processes\n", argv[0], getpid());

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
	if(debug) printf("\t==> DEBUG[%s - %d]: Parent working\n", own_name, getpid());


	int child_status = 0;
	int child_pid = 0;

	while((child_pid = wait(&child_status)) >= 0)
	{
		if(child_status != 0)
		{
			printf("\t%s - ERROR: Unexpected child process return (value %d ; pid %d))\n", own_name, child_status, child_pid);
			//return child_status;
		}
	}

	return 0;
}

int child_action(char *shname, int key)
{
	printf("\t==> Client %d starting\n", getpid());

	////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////// Remap shared memory to revalidate pointers //////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////

	shop_t *shop = child_remap_shmem(shname, key);
	if(shop == NULL) return 1;

	if(debug) printf("\t==> DEBUG[%s - %d]: Child working\n", own_name, getpid());

	////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////// Create FIFO ///////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////

	char* fifo_pathname = get_fifo_pathname(getpid());

	if(debug) printf("\t==> DEBUG[%s - %d]: Constructed fifo_pathname \"%s\"\n", own_name, getpid(), fifo_pathname);

	if(mkfifo(fifo_pathname, CL_FIFO_MODE) != 0)
	{
		printf("Error: unable to create client FIFO.\n");
		return 1;
	}

	if(debug) printf("\t==> DEBUG[%s - %d]: First loja lock\n", own_name, getpid());

	////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////// Lock loja mutex ////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////

	if(attempt_mutex_lock(&(shop->loja_mutex), "loja", debug) != 0) return 1;

	if(debug) printf("\t==> DEBUG[%s - %d]: Accessing loja\n", own_name, getpid());

	size_t num_balcoes = shop->num_balcoes;

	if(num_balcoes > 0)
	{

		////////////////////////////////////////////////////////////////////////////////////////////////
		//////////////////////////// Choose the balcao with less clients ///////////////////////////////
		////////////////////////////////////////////////////////////////////////////////////////////////

		size_t min_occup_index = -1;
		int balcao_fifo_fd = -1;

		if(debug) printf("\t==> DEBUG[%s - %d]: Choosing balcao\n", own_name, getpid());

		choose_best_balcao(shop, num_balcoes, &min_occup_index);

		if(min_occup_index == -1)
		{
			printf("\t==> ERROR: client %d found no open counter to be attended at\n", getpid());
			attempt_mutex_unlock(&(shop->loja_mutex), "loja", debug);
			return 1;
		}

		if(debug) printf("\t==> DEBUG[%s - %d]: Chosen balcao %d\n", own_name, getpid(), shop->balcoes[min_occup_index].num);

		////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////// Lock the balcao for access ///////////////////////////////////
		////////////////////////////////////////////////////////////////////////////////////////////////

		if(attempt_mutex_lock(&(shop->balcoes[min_occup_index].balcao_mutex), "balcao", debug) != 0)
		{
			attempt_mutex_unlock(&(shop->loja_mutex), "loja", debug);
			return 1;
		}

		shop->balcoes[min_occup_index].clientes_em_atendimento++;

		////////////////////////////////////////////////////////////////////////////////////////////////
		//////////////////////////////////// Write to balcao FIFO //////////////////////////////////////
		////////////////////////////////////////////////////////////////////////////////////////////////

		char *fifo_name = shop->balcoes[min_occup_index].fifo_name;
		char path[strlen(FIFO_DIR) + strlen(fifo_name) + 1];
		strcpy(path, FIFO_DIR);
		strcat(path, fifo_name);

		if(debug) printf("\t==> DEBUG[%s - %d]: Balcao path[%s]\n", own_name, getpid(), path);

		balcao_fifo_fd = open(path, O_WRONLY);
		if(debug) printf("\t==> DEBUG[%s - %d]: Successfully opened balcao FIFO %s for writting.\n", own_name, getpid(), path);
		if(balcao_fifo_fd < 0)
		{
			printf("Error: unable to open \"balcao\" FIFO.\n");

			attempt_mutex_unlock(&(shop->balcoes[min_occup_index].balcao_mutex), "balcao", debug);
			attempt_mutex_unlock(&(shop->loja_mutex), "loja", debug);
			return 1;
		}

		if(write(balcao_fifo_fd, fifo_pathname, MAX_FIFO_NAME_LEN) != MAX_FIFO_NAME_LEN)
		{
			printf("Error: problems writing to \"balcao\" FIFO.\n");

			attempt_mutex_unlock(&(shop->balcoes[min_occup_index].balcao_mutex), "balcao", debug);
			attempt_mutex_unlock(&(shop->loja_mutex), "balcao", debug);

			return 1;
		}

		if (write_log_entry(shname, CLIENT, min_occup_index + 1, "pede_atendimento", shop->balcoes[min_occup_index].fifo_name))
		{
			printf("Warning: could not write to logfile.\n");
		}

		printf("\t==> Client %d attended by balcao %d\n", getpid(), shop->balcoes[min_occup_index].num);

		////////////////////////////////////////////////////////////////////////////////////////////////
		////////////////////////////////// Unlock balcao mutex /////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////////////////////////////

		if(attempt_mutex_unlock(&(shop->balcoes[min_occup_index].balcao_mutex), "balcao", debug) != 0) return 1;

		////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////// Wait for end of attendance ///////////////////////////////////
		////////////////////////////////////////////////////////////////////////////////////////////////

		int fifo_read = open(fifo_pathname, O_RDONLY);

		////////////////////////////////////////////////////////////////////////////////////////////////
		//////////////////////////////////// Unlock loja mutex /////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////////////////////////////

		if(attempt_mutex_unlock(&(shop->loja_mutex), "loja", debug) != 0) return 1;

		if(debug) printf("\t==> DEBUG[%s - %d]: Created fifo read(%d, %s)\n", own_name, getpid(), fifo_read, path);

		if(fifo_read < 0)
		{
			printf("Error: unable to open client FIFO for reading.\n");
			return 1;
		}

		if(debug) printf("\t==> DEBUG[%s - %d]: FIFO opening successful\n", own_name, getpid());

		char balcao_message[MAX_FIFO_NAME_LEN];

		if(debug) printf("\t==> DEBUG[%s - %d]: Read from balcao\n", own_name, getpid());

		while(1)
		{
			read(fifo_read, balcao_message, MAX_FIFO_NAME_LEN);

			if(strcmp(balcao_message, ATTEND_END_MESSAGE) == 0)
			{
				if (write_log_entry(shname, CLIENT, min_occup_index + 1, "fim_atendimento", fifo_pathname))
				{
					printf("Warning: could not write to logfile.\n");
				}
				break;
			}
		}

		close(balcao_fifo_fd);

		close(fifo_read);
		if(unlink(fifo_pathname) != 0) {
			printf("Error: unable to unlink client FIFO.\n");
			return 1;
		}
		free(fifo_pathname);
	}
	else
	{
		if(attempt_mutex_unlock(&(shop->loja_mutex), "loja", debug) != 0)
			printf("Error: unable to unlock \"loja\" mutex.\n");

		printf("Error: the specified shop has no valid counters.\n");
		return 1;
	}

	if(debug) printf("\t==> DEBUG[%s - %d]: Finishing well\n", own_name, getpid());

	printf("\t==> Client %d terminating with success\n", getpid());

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

void choose_best_balcao(shop_t *shop, const int num_balcoes, size_t *min_occup_index)
{
	int i;
	int min_occup = INT_MAX;
	for(i = 0; i < num_balcoes; i++)
	{
		if((shop->balcoes[i].duracao == -1) && (shop->balcoes[i].clientes_em_atendimento < min_occup))
		{
			min_occup = shop->balcoes[i].clientes_em_atendimento;
			*min_occup_index = i;
		}
	}
}
