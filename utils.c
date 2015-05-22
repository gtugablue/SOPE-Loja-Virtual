#include "utils.h"
#include <limits.h>

int parse_int(int* dest, char* str, int base)
{
	long result;
	if (parse_long(&result, str, base)) return 1;

	if (result > INT_MAX) return 1;

	*dest = result;
	return 0;
}

int parse_long(long* dest, char* str, int base)
{
	char* test;
	long value;
	value = strtol(str, &test, base);

	if((*test != '\0') || errno == ERANGE)
		return 1;

	*dest = value;

	return 0;
}

char *filenameFromPath(const char* path)
{
	int size = strlen(path);
	char *result = malloc(size + 1);

	size_t i = 0;
	size_t curr_index = 0;
	for(; i < size; i++)
	{
		if(path[i] == '/')
		{
			curr_index = 0;
			continue;
		}

		result[curr_index++] = path[i];
	}
	result[curr_index] = '\0';
	return result;
}

int attempt_mutex_lock(pthread_mutex_t *mutex, char *name, int debug)
{
	/*if(debug) printf("Locking mutex 0x%X by %d\n", (unsigned)(unsigned long)mutex, getpid());
	if(pthread_mutex_lock(mutex) != 0)
	{
		//if(debug) printf("Error: unable to lock \"%s\" mutex.\n", name);
		if(debug) printf("Error: unable to lock \"%s\" mutex.\n", name);
		return 1;
	}
	if(debug) printf("Locked mutex 0x%X by %d\n", (unsigned)(unsigned long)mutex, getpid());*/
	return 0;
}

int attempt_mutex_unlock(pthread_mutex_t *mutex, char *name, int debug)
{
	/*if(debug) printf("Unlocking mutex 0x%X by %d\n", (unsigned)(unsigned long)mutex, getpid());
	if(pthread_mutex_unlock(mutex) != 0)
	{
		//if(debug) printf("Error: unable to unlock \"%s\" mutex.\n", name);
		if(debug) printf("Error: unable to unlock \"%s\" mutex.\n", name);
		return 1;
	}
	if(debug) printf("Unlocked mutex 0x%X by %d\n", (unsigned)(unsigned long)mutex, getpid());*/
	return 0;
}

int attempt_mutex_destroy(pthread_mutex_t *mutex, char *name, int debug)
{
	if(debug) printf("Destroying mutex 0x%X by %d\n", (unsigned)(unsigned long)mutex, getpid());
	if(pthread_mutex_destroy(mutex) != 0)
	{
		//if(debug) printf("Error: unable to destroy \"%s\" mutex.\n", name);
		if(debug) printf("Error: unable to destroy \"%s\" mutex.\n", name);
		return 1;
	}
	if(debug) printf("Destroyed mutex 0x%X by %d\n", (unsigned)(unsigned long)mutex, getpid());
	return 0;
}
