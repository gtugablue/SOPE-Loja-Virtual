#include "utils.h"

int parse_int(long* dest, char* str, int base)
{
	char* test;
	long value;
	value = strtol(str, &test, base);

	if((*test != '\0') || errno == ERANGE)
		return 1;

	*dest = value;

	return 0;
}
