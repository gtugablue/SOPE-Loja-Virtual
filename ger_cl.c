#include "ger_cl.h"

int main(int argc, char **argv)
{
	if(argc != 3)
	{
		printf("\n\t%s: Invalid number of arguments.\n\tMust be: %s <shared_memory_name> <number_of_clients>\n\n", argv[0], argv[0]);
		return 1;
	}

	int mem_key;
	int shm_id;
	int num_clients;

	if((parse_int(&mem_key, argv[1], 10) != 0) || (shm_id = shmget(mem_key, SHM_SIZE, 0)) == -1)
	{
		printf("\n\t%s - ERROR: Invalid shared memory key. That\n\tregion is not open or the value is not a number\n\n", argv[0]);
		return 1;
	}

	if(parse_int(&num_clients, argv[2], 10) != 0)
	{
		printf("\n\t%s - ERROR: Invalid number of clients.\n\tThat value is not a number\n\n", argv[0]);
		return 1;
	}

	return 0;
}

int parse_int(int* dest, char* str, int base)
{
	char* test;
	int value;
	value = strtol(str, &test, base);

	if((*test != '\0') || errno == ERANGE)
		return 1;

	*dest = value;

	return 0;
}
