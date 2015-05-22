#ifndef UTILS_H_
#define UTILS_H_

#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>

int parse_int(int* dest, char* str, int base);	// returns 0 if str is a number, 1 otherwise
int parse_long(long* dest, char* str, int base);	// returns 0 if str is a number, 1 otherwise
char *filenameFromPath(const char* path);

int attempt_mutex_lock(pthread_mutex_t *mutex, char *name, int debug);
int attempt_mutex_unlock(pthread_mutex_t *mutex, char *name, int debug);
int attempt_mutex_destroy(pthread_mutex_t *mutex, char *name, int debug);

#endif
