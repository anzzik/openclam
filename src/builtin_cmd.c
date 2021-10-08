#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "builtin_cmd.h"
#include "shell.h"
#include "job.h"

static int bi_bg(Cmd_t *c, void *u_ptr, int fd_in, int fd_out, int fd_err);
static int bi_cd(Cmd_t *c, void *u_ptr, int fd_in, int fd_out, int fd_err);
static int bi_fg(Cmd_t *c, void *u_ptr, int fd_in, int fd_out, int fd_err);
static int bi_jobs(Cmd_t *c, void *u_ptr, int fd_in, int fd_out, int fd_err);

typedef struct BuiltInCmdDef_s BuiltInCmdDef_t;
struct BuiltInCmdDef_s
{
	char *cmd_name;
	int (*cmd_fn)(Cmd_t*,void*,int,int,int);
};

static BuiltInCmdDef_t bi_cmds[] = {
	{ "bg", bi_bg },
	{ "cd", bi_cd },
	{ "fg", bi_fg },
	{ "jobs", bi_jobs },
	{ "", NULL }
};

static int bi_bg(Cmd_t *c, void *u_ptr, int fd_in, int fd_out, int fd_err)
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

static int bi_cd(Cmd_t *c, void *u_ptr, int fd_in, int fd_out, int fd_err)
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

static int bi_fg(Cmd_t *c, void *u_ptr, int fd_in, int fd_out, int fd_err)
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

static int bi_jobs(Cmd_t *c, void *u_ptr, int fd_in, int fd_out, int fd_err)
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

int builtin_call(Cmd_t *c, void *u_ptr, int fd_in, int fd_out, int fd_err)
{
	int r;

	for (int i = 0; bi_cmds[i].cmd_fn; i++)
	{
		if (!strcmp(bi_cmds[i].cmd_name, c->argv[0]))
		{
			r = bi_cmds[i].cmd_fn(c, u_ptr, fd_in, fd_out, fd_err);
			return r;
		}
	}

	return -1;
}

int builtin_cmd_exists(Cmd_t *c)
{
	for (int i = 0; bi_cmds[i].cmd_fn; i++)
		if (!strcmp(bi_cmds[i].cmd_name, c->argv[0]))
			return 1;

	return 0;
}

