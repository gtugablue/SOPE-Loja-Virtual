#include <stdio.h>
#include "utils.h"
#include <sys/shm.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <unistd.h>

#define SHARED_MEM_SIZE	1024 * 1024

int create_shared_memory(int *shm_id);

int main(int argc, char *argv[])
{
	int shm_id;
	if (create_shared_memory(&shm_id)) return 1;

	if (shmctl(shm_id, IPC_RMID, NULL) == -1)
	{
		printf("Error: shared memory wasn't properly cleaned.\n");
		return 1;
	}

	printf("Hello world!\n");
	return 0;
}

int create_shared_memory(int *shm_id)
{
	char pathname[PATH_MAX];
	if (getcwd(pathname, PATH_MAX) == NULL)
	{
		printf("Error: couldn't obtain current path.\n");
		return 1;
	}
	key_t shared_mem_key = ftok(pathname, 0);
	if (shared_mem_key == -1)
	{
		printf("Error: couldn't obtain a key for the shared memory\n.");
		return 1;
	}
	if ((*shm_id = shmget(shared_mem_key, SHARED_MEM_SIZE, IPC_CREAT | IPC_EXCL)) == -1)
	{
		printf("Error: couldn't create shared memory.\n");
		return 1;
	}
	return 0;
}
