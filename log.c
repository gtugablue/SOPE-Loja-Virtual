#include "log.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int initialize_log(const char *sh_name)
{
	char extension[] = ".log";
	char file_name[strlen(sh_name) + strlen(extension) + 1];
	strcpy(file_name, sh_name);
	strcat(file_name, extension);
	FILE *fp = fopen(file_name, "w");
	if (fp == NULL) return 1;

	if (fprintf(fp, "%-20s | %-6s | %-7s | %-22s | %-18s\n", "quando", "quem", "balcao", "o_que", "canal_criado/usado") < 0) return 1;
	unsigned num_hifens = 20 + 6 + 7 + 22 + 18 + 4 * 3;
	char hifens[num_hifens + 2];
	size_t i;
	for (i = 0; i < num_hifens; ++i)
	{
		hifens[i] = '-';
	}
	hifens[num_hifens] = '\n';
	hifens[num_hifens] = '\0';
	if (fprintf(fp, "%s", hifens) < 0) return 1;
	if (fclose(fp) != 0) return 1;
	return 0;
}

int write_log_entry()
{

}
