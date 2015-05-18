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

	long mem_key;
	int shm_id;
	long num_clients;

	if((parse_long(&mem_key, argv[1], 10) != 0) || (shm_id = shmget(mem_key, SHARED_MEM_SIZE, 0)) == -1)
	{
		printf("\n\t%s - ERROR: Invalid shared memory key. That\n\tregion is not open or the value is not a number\n\n", argv[0]);
		//return 1;
	}

	if(parse_long(&num_clients, argv[2], 10) != 0)
	{
		printf("\n\t%s - ERROR: Invalid number of clients.\n\tThat value is not a number\n\n", argv[0]);
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
		return child_action();
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

int child_action()
{
	int pid = getpid();
	char* pid_str = malloc(MAX_FIFO_NAME_LEN);
	sprintf(pid_str, "%d", pid);

	char* fifo_pathname = malloc(MAX_FIFO_NAME_LEN);
	strcpy(fifo_pathname, CL_FIFO_NAME);
	strcpy(fifo_pathname + strlen(CL_FIFO_NAME), pid_str);		// fifo_pathname now has the complete path for the fifo

	free(pid_str);

	if(mkfifo(fifo_pathname, CL_FIFO_MODE) != 0)
		return 1;

	// TODO Read "loja" from shared memory
	// TODO Choose best "balcao" to be attended at
	// TODO "Freeze" on that balcao until the "fim_atendimento" message is received

	return 0;
}
