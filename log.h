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

/*
 * @brief Initializes the log file
 * @param sh_name name of the shared memory
 * @return 0 on success, otherwise means errors occurred
 */
int initialize_log(const char *sh_name);

/*
 * @brief Writes a new line to the log file
 * @param sh_name name of the shared memory
 * @param who object of the struct t_log_who, specifying the source of the log entry
 * @param balcaoID number of the balcao involved in the event
 * @param what action to be logged
 * @param channel_created_used name of the channel created or used in the event
 * @return 0 on success, otherwise means errors occurred
 */
int write_log_entry(const char *sh_name, t_log_who who, unsigned balcaoID, const char *what, const char *channel_created_used);

#endif
