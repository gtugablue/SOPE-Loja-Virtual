#ifndef UTILS_H_
#define UTILS_H_

#include <stdlib.h>
#include <errno.h>

int parse_int(long* dest, char* str, int base);	// returns 0 if str is a number, 1 otherwise

#endif
