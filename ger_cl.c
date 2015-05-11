#include "ger_cl.h"

int main(int argc, char **argv)
{
	if(argc != 3)
	{
		printf("\n\t%s: Invalid number of arguments.\n\tMust be: %s <shared_memory_name> <number_of_clients>\n\n", argv[0], argv[0]);
		return 1;
	}

	long mem_key;
	int shm_id;
	long num_clients;

	if((parse_long(&mem_key, argv[1], 10) != 0) || (shm_id = shmget(mem_key, SHM_SIZE, 0)) == -1)
	{
		printf("\n\t%s - ERROR: Invalid shared memory key. That\n\tregion is not open or the value is not a number\n\n", argv[0]);
		return 1;
	}

	if(parse_long(&num_clients, argv[2], 10) != 0)
	{
		printf("\n\t%s - ERROR: Invalid number of clients.\n\tThat value is not a number\n\n", argv[0]);
		return 1;
	}

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

	if(fork_return == 0)	// Child-processes - each represents a client who chooses a "balcao" to be attended at
	{
		// TODO Something
	}
	else					// Parent-process - must wait for each child to finnish before terminating
	{
		int child_status = 0;

		while(wait(&child_status) >= 0)
		{
			if(child_status != 0)
			{
				return 3;
			}
		}
	}

	return 0;
}
