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

char *filenameFromPath(char* path)
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
