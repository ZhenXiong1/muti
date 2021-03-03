
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "Pool.h"
#include "Service.h"
#include "Stat.h"
#include "Storage.h"
#include "Ut.h"

typedef int(*CommandFn)(int argv, char **argvs);

typedef struct Command {
	char		*name;
	CommandFn	cmd;
	char		*desc;
} Command;

Command MutiCmds[] = {
	{"ut", UtDo, "Do internal unit tests, only for develop person."},
	{"service", ServiceDo, "Start all kinds of services(OSD, MDS, MON ...)."},
	{"pool", PoolDo, "Pool related operations(create, list, destroy ...)."},
	{"storage", StorageDo, "Storage related operations(put, get, delete ...)."},
	{"stat", StatDo, "Statistic related operations(Cluster, nodes ...)."},
	{(char *)NULL, (CommandFn)NULL, (char*)NULL}
};

Command *findCommand(char *cnm) {
        int i = 0;
        while (MutiCmds[i].name != NULL) {
                if (strcasecmp(MutiCmds[i].name, cnm) == 0) {
                        return &MutiCmds[i];
                }
                i ++;
        }
        return NULL;
}

int main(int argv, char **argvs) {
        int ret = 0;
        int i = 0, j, maxlen;

        if (argv > 1) {
        	Command *tcase = findCommand(argvs[1]);
		if (tcase != NULL) {
			ret = (*tcase->cmd)(argv, argvs);
		} else {
			printf("Unknown command:%s\n", argvs[1]);
			goto wrong_cmd;
		}
        } else {
wrong_cmd:
                printf("Usage:./muti <command>\n\n");
                printf("These are common muti commands used in various situations:\n\n");

                maxlen = 0;
                i = 0;
                while (MutiCmds[i].name != NULL) {
                	if (strlen(MutiCmds[i].name) > maxlen) maxlen = strlen(MutiCmds[i].name);
                	i++;
                }
                i = 0;
                while (MutiCmds[i].name != NULL) {
                        printf("   %s   ", MutiCmds[i].name);
                        for(j = strlen(MutiCmds[i].name); j < maxlen; j++) printf(" ");
                        printf("%s\n", MutiCmds[i].desc);
                        i++;
                }
        }
        return ret;
}
