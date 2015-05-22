#ifndef LOG_H
#define LOG_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "utils.h"

typedef enum
{
	BALCAO,
	CLIENT
} t_log_who;

int initialize_log(const char *sh_name);
int write_log_entry(const char *sh_name, t_log_who who, unsigned balcaoID, const char *what, const char *channel_created_used);

#endif
