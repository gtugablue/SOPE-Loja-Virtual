#include "ger_cl.h"

int main(int argc, char **argv)
{

	////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////// Check if arguments are valid and initialize variables /////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////


	if(argc != 3)
	{
		printf("\n\t%s: Invalid number of arguments.\n\tMust be: %s <shared_memory_name> <number_of_clients>\n\n", argv[0], argv[0]);
		return 1;
	}

	long num_clients;

	if(parse_long(&num_clients, argv[2], 10) != 0)
	{
		printf("\n\t%s - ERROR: Invalid number of clients.\n\tThat value is not a number\n\n", argv[0]);
		return 1;
	}

	shop_t *shop = NULL;
	int shm_key = 0;
	if(retrieve_shop(shop, &shm_key, argv[1]) != 0)
	{
		printf("\n\t%s - ERROR: Unable to open the specified memory region\n\n", argv[0]);
		return 1;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////// Initialize child processes /////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////

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
	/////////////// Give each process it's task (parent waits, child becomes client ////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////

	if(fork_return == 0)	// Child-processes - each represents a client who chooses a "balcao" to be attended at
		return child_action(shop);
	else					// Parent-process - must wait for each child to finnish before terminating
		return parent_action();

	return 0;
}

int parent_action()
{
	int child_status = 0;

	while(wait(&child_status) >= 0)
	{
		if(child_status != 0)
		{
			return 3;
		}
	}

	return 0;
}

int child_action(shop_t *shop)
{
	int pid = getpid();
	char* pid_str = malloc(MAX_FIFO_NAME_LEN);
	sprintf(pid_str, "%d", pid);

	char fifo_pathname[MAX_FIFO_NAME_LEN];
	strcpy(fifo_pathname, CL_FIFO_NAME);
	strcpy(fifo_pathname + strlen(CL_FIFO_NAME), pid_str);		// fifo_pathname now has the complete path for the fifo

	free(pid_str);

	if(mkfifo(fifo_pathname, CL_FIFO_MODE) != 0)
	{
		printf("Error: unable to create client FIFO.\n");
		return 1;
	}

	int fifo_write = open(fifo_pathname, O_WRONLY | O_NONBLOCK);
	int fifo_read = open(fifo_pathname, O_RDONLY);
	if(fifo_read < 0 || fifo_write < 0)
	{
		printf("Error: unable to open client FIFO for reading.\n");
		return 1;
	}

	int fifo_pathname_len = strlen(fifo_pathname);
	fifo_pathname[fifo_pathname_len++] = '\0';
	char balcao_message[MAX_FIFO_NAME_LEN];

	int min_occup = INT_MAX;
	size_t min_occup_index = -1;
	size_t i;
	int balcao_fifo_fd = -1;

	if(pthread_mutex_lock(&shop->loja_mutex) != 0)
	{
		printf("Error: unable to lock \"loja\" mutex.\n");
		return 1;
	}

	size_t num_balcoes = shop->num_balcoes;

	if(num_balcoes > 0)
	{
		for(i = 0; i < num_balcoes; i++)
		{
			if(shop->balcoes[i].duracao == -1)
			{
				if(shop->balcoes[i].clientes_em_atendimento < min_occup)
				{
					min_occup = shop->balcoes[i].clientes_em_atendimento;
					min_occup_index = i;
				}
			}
		}

		if(pthread_mutex_lock(&shop->balcoes[min_occup_index].balcao_mutex) != 0)
		{
			printf("Error: unable to lock \"balcao\" mutex.\n");

			if(pthread_mutex_unlock(&shop->loja_mutex) != 0)
			{
				printf("Error: unable to unlock \"loja\" mutex.\n");
			}
			return 1;
		}

		balcao_fifo_fd = open(shop->balcoes[min_occup_index].fifo_name, O_WRONLY);

		if(balcao_fifo_fd < 0)
		{
			printf("Error: unable to open \"balcao\" FIFO.\n");

			if(pthread_mutex_unlock(&shop->balcoes[min_occup_index].balcao_mutex) != 0)
			{
				printf("Error: unable to unlock \"balcao\" mutex.\n");
			}

			if(pthread_mutex_unlock(&shop->loja_mutex) != 0)
			{
				printf("Error: unable to unlock \"loja\" mutex.\n");
			}

			return 1;
		}

		if(write(balcao_fifo_fd, fifo_pathname, fifo_pathname_len) != fifo_pathname_len)
		{
			printf("Error: problems writing to \"balcao\" FIFO.\n");

			if(pthread_mutex_unlock(&shop->balcoes[i].balcao_mutex) != 0)
			{
				printf("Error: unable to unlock \"balcao\" mutex.\n");
			}

			if(pthread_mutex_unlock(&shop->loja_mutex) != 0)
			{
				printf("Error: unable to unlock \"loja\" mutex.\n");
			}

			return 1;
		}

		if(pthread_mutex_unlock(&shop->loja_mutex) != 0)
		{
			printf("Error: unable to unlock \"loja\" mutex.\n");
			return 1;
		}

		while(1)
		{
			read(fifo_read, balcao_message, MAX_FIFO_NAME_LEN);

			if(strcmp(balcao_message, ATTEND_END_MESSAGE) == 0)
				break;
		}

		close(fifo_read);
		close(fifo_write);

		if(unlink(fifo_pathname) != 0)
		{
			printf("Error: unable to unlink client FIFO.\n");
			return 1;
		}
	}
	else
	{
		if(pthread_mutex_unlock(&shop->loja_mutex) != 0)
			printf("Error: unable to unlock \"loja\" mutex.\n");

		printf("Error: the shop specified has no valid balcons.\n");
		return 1;
	}

	return 0;
}

int retrieve_shop(shop_t *shop, int *key, char *shm_name)
{
	int result = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, SHARED_MEM_MODE);

	if(result < 0)
		return 1;

	*key = result;
	shop = (shop_t *) mmap(0,SHARED_MEM_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,*key,0);

	return 0;
}

