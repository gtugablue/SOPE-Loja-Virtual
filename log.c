#include "log.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define EXTENSION ".log"

void get_logfile_name(const char *sh_name, char *logfile_name);
const char *get_creator_name(t_log_who who);

int initialize_log(const char *sh_name)
{
	char file_name[strlen(sh_name) + strlen(EXTENSION) + 1];
	get_logfile_name(sh_name, file_name);
	FILE *fp = fopen(file_name, "w");
	if (fp == NULL) return 1;

	if (fprintf(fp, "%-19s | %-6s | %-7s | %-22s | %-18s\n", "quando", "quem", "balcao", "o_que", "canal_criado/usado") < 0) return 1;
	unsigned num_hifens = 19 + 6 + 7 + 22 + 18 + 4 * 3;
	char hifens[num_hifens + 2];
	size_t i;
	for (i = 0; i < num_hifens; ++i)
	{
		hifens[i] = '-';
	}
	hifens[num_hifens] = '\n';
	hifens[num_hifens + 1] = '\0';
	if (fprintf(fp, "%s", hifens) < 0) return 1;
	if (fclose(fp) != 0) return 1;
	return 0;
}

int write_log_entry(const char *sh_name, t_log_who who, unsigned balcaoID, const char *what, const char *channel_created_used)
{
	time_t t = time(NULL);
	if (t == -1) return 1;
	struct tm *tstruct = localtime(&t);

	char date[20 + 1];
	if (strftime(date, 20, "%F %T", tstruct) == 0) return 1;

	char file_name[strlen(sh_name) + strlen(EXTENSION) + 1];
	get_logfile_name(sh_name, file_name);

	FILE *fp = fopen(file_name, "a");
	if (fp == NULL) return 1;
	if (fprintf(fp, "%-19s | %-6s | %-7d | %-22s | %-18s\n", date, get_creator_name(who), balcaoID, what, channel_created_used) < 0) return 1;
	if (fclose(fp) != 0) return 1;

	return 0;
}

void get_logfile_name(const char *sh_name, char *logfile_name)
{
	strcpy(logfile_name, sh_name);
	strcat(logfile_name, EXTENSION);
}

const char *get_creator_name(t_log_who who)
{
	switch (who)
	{
	case BALCAO: return "Balcao";
	case CLIENT: return "Client";
	default: return NULL;
	}
}
