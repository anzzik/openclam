#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "internal_cmds.h"

static int internal_fg(void *arg, int fd_in, int fd_out, int fd_err);

static InternalCmdDef_t icmds[] = {
	{ "fg", internal_fg },
	{ "", NULL }
};

static int internal_fg(void *arg, int fd_in, int fd_out, int fd_err)
{
	FILE *f = fdopen(fd_out, "w");

	fwrite("Hello\n", 1, 6, f);
	fflush(f);

	return 0;
}

int internal_call(const char* cmd_name, void *u_arg, int fd_in, int fd_out, int fd_err)
{
	int r;

	for (int i = 0; icmds[i].cmd_fn; i++)
	{
		if (!strcmp(icmds[i].cmd_name, cmd_name))
		{
			r = icmds[i].cmd_fn(NULL, fd_in, fd_out, fd_err);
			return r;
		}
	}

	return -1;
}


