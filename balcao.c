#include <stdio.h>
#include "utils.h"
#include <sys/shm.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define SHARED_MEM_SIZE	1024 * 1024

int create_shared_memory(const char *name, int *shm_id);

int main(int argc, char *argv[])
{
	int shm_id;
	if (create_shared_memory(argv[1], &shm_id)) return 1;

	if (shm_unlink(argv[1]) == -1)
	{
		printf("Error: shared memory wasn't properly cleaned.\n");
		return 1;
	}

	printf("Hello world!\n");
	return 0;
}

int create_shared_memory(const char *name, int *shm_id)
{
	if ((*shm_id = shm_open(name, O_CREAT | O_EXCL | O_RDWR, 0600)) == -1)
	{
		printf("Error: couldn't create shared memory.\n");
		return 1;
	}
	return 0;
}
