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
