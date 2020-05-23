#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "internal_cmds.h"
#include "shell.h"
#include "job.h"

static int internal_bg(Cmd_t *c, void *u_ptr, int fd_in, int fd_out, int fd_err);
static int internal_cd(Cmd_t *c, void *u_ptr, int fd_in, int fd_out, int fd_err);
static int internal_fg(Cmd_t *c, void *u_ptr, int fd_in, int fd_out, int fd_err);
static int internal_jobs(Cmd_t *c, void *u_ptr, int fd_in, int fd_out, int fd_err);

static InternalCmdDef_t icmds[] = {
	{ "bg", internal_bg },
	{ "cd", internal_cd },
	{ "fg", internal_fg },
	{ "jobs", internal_jobs },
	{ "", NULL }
};

static int internal_bg(Cmd_t *c, void *u_ptr, int fd_in, int fd_out, int fd_err)
{
	FILE *f = fdopen(fd_out, "w");
	Shell_t *sh = u_ptr;
	Job_t *j = sh->first_j;

	for (j = sh->first_j; j; j = j->next)
	{
		if (job_is_stopped(j))
		{
			fprintf(f, "Continuing a suspended job\n");
			fflush(f);

			job_continue(j);
			return 0;
		}
	}

	fprintf(f, "No background jobs\n");
	fflush(f);

	return 0;
}

static int internal_cd(Cmd_t *c, void *u_ptr, int fd_in, int fd_out, int fd_err)
{
	FILE *f = fdopen(fd_out, "w");

	if (c->argc > 2)
	{
		fprintf(f, "cd takes max 1 argument\n");
		fflush(f);

		return -1;
	}

	if (c->argc == 2)
	{
		if (chdir(c->argv[1]) < 0)
			perror("cd");
	}

	if (c->argc == 1)
	{
		if (chdir(getenv("HOME")) < 0)
			perror("cd");
	}

	return 0;
}

static int internal_fg(Cmd_t *c, void *u_ptr, int fd_in, int fd_out, int fd_err)
{
	FILE *f = fdopen(fd_out, "w");
	Shell_t *sh = u_ptr;
	Job_t *j = sh->first_j;

	for (j = sh->first_j; j; j = j->next)
	{
		if (job_is_bg(j))
		{
			job_put_fg(j, 1);
			return 0;
		}
	}

	fprintf(f, "No background jobs\n");
	fflush(f);

	return 0;
}

static int internal_jobs(Cmd_t *c, void *u_ptr, int fd_in, int fd_out, int fd_err)
{
	FILE *f = fdopen(fd_out, "w");
	Shell_t *sh = u_ptr;
	int i = 0;

	Job_t *j = sh->first_j;

	for (j = sh->first_j; j; j = j->next)
	{
		if (job_is_bg(j))
		{
			fprintf(f, "%d: %s\n", i++, j->first_c->argv[0]);
		}
	}

	fflush(f);

	return 0;
}

int internal_call(Cmd_t *c, void *u_ptr, int fd_in, int fd_out, int fd_err)
{
	int r;

	for (int i = 0; icmds[i].cmd_fn; i++)
	{
		if (!strcmp(icmds[i].cmd_name, c->argv[0]))
		{
			r = icmds[i].cmd_fn(c, u_ptr, fd_in, fd_out, fd_err);
			return r;
		}
	}

	return -1;
}

int internal_cmd_exists(Cmd_t *c)
{
	for (int i = 0; icmds[i].cmd_fn; i++)
		if (!strcmp(icmds[i].cmd_name, c->argv[0]))
			return 1;

	return 0;
}

