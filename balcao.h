#include <stdio.h>
#include "utils.h"
#include <sys/shm.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <unistd.h>

#define SHARED_MEM_SIZE	(1024 * 1024)

int main(int argc, char *argv[]);
int create_shared_memory(int *shm_id);
