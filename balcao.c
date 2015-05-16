#include <stdio.h>
#include "utils.h"
#include <sys/shm.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "shop_info.h"

#define SHARED_MEM_SIZE	1024 * 1024

int create_shared_memory(const char *name, int *shm_id, long size);

int main(int argc, char *argv[])
{
	int shm_id;
	if (create_shared_memory(argv[1], &shm_id, SHARED_MEM_SIZE)) return 1;

	if (shm_unlink(argv[1]) == -1)
	{
		printf("Error: shared memory wasn't properly cleaned.\n");
		return 1;
	}

	printf("Hello world!\n");
	return 0;
}

int create_shared_memory(const char *name, int *shm_id, long size)
{
	if ((*shm_id = shm_open(name, O_CREAT | O_EXCL | O_RDWR, 0600)) == -1)
	{
		printf("Error: couldn't create shared memory.\n");
		return 1;
	}
	if (ftruncate(*shm_id, size) == -1)
	{
		printf("Error: couldn't allocate space in the shared memory.\n");
	}
	return 0;
}
