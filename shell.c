#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "shell.h"
#include "cmd.h"
#include "builtin_cmd.h"

static Shell_t *sh;

int shell_init()
{
	sh = malloc(sizeof(Shell_t));

	sh->shell_fd = STDIN_FILENO;
	sh->pid      = getpid();
	sh->pgid     = getpgrp();

	if (!isatty(sh->shell_fd))
	{
		fprintf(stderr, "%s\n", "shell is NOT interactive");
		sh->interactive = 0;

		return -1;
	}

	sh->fg_pgid = tcgetpgrp(sh->shell_fd);
	sh->interactive = 1;

	while (sh->fg_pgid != sh->pgid)
	{
		fprintf(stderr, "shell is not in foreground, have to stop it until fg..\n");
		kill(-1 * sh->pgid, SIGTTIN); // this puts the process group in T state
		
		sh->fg_pgid = tcgetpgrp(sh->shell_fd);
	}

	signal(SIGINT,  SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
//	signal(SIGCHLD, SIG_IGN);

	if (setpgid(sh->pid, sh->pid) < 0)
	{
		perror("Couldn't put the shell in its own process group");
		return -1;
	}

	sh->pgid = sh->pid;

	tcsetpgrp(sh->shell_fd, sh->pid);
	tcgetattr(sh->shell_fd, &sh->tmodes);
	memcpy(&sh->def_tmodes, &sh->tmodes, sizeof(struct termios));

	return 0;
}

void shell_push_job(Job_t *j)
{
	Job_t **ptr;

	ptr = &(sh->first_j);

	while (*ptr)
		ptr = &((*ptr)->next);

	*ptr = j;
}

void shell_free()
{
	free(sh);
}

int shell_builtin_cmd(Cmd_t *c, int fd_in, int fd_out, int fd_err)
{
	(void)builtin_call(c, sh, fd_in, fd_out, fd_err);

	return 0;
}

int shell_get_cmdline(char *buf, int n)
{
	int read_c;
	char fmt_str[10];
	char *cwd = NULL;

	cwd = getcwd(cwd, 0);

	printf("%s > ", cwd);
	fflush(stdout);
	free(cwd);

	sprintf(fmt_str, "%%%d[^\n]", n - 1);

	read_c = scanf(fmt_str, buf);
	getchar();

	return read_c;
}

void shell_free_tmp_argv(int argc, char **argv)
{
	for (int i = 0; i < argc; i++)
		free(argv[i]);
}

void shell_cleanup()
{
	Job_t **ptr;
	Job_t *tmp;

	ptr = &sh->first_j;

	while (*ptr)
	{
		if (job_is_completed(*ptr))
		{
			tmp = *ptr;
			*ptr = (*ptr)->next;
			job_free(tmp);

			continue;
		}

		ptr = &((*ptr)->next);
	}
}

int shell_mainloop()
{
	Job_t *j;
	Cmd_t *cmd;
	char  buf[255] = { '\0' };

	int   cmdc = 0;
	char *cmds[255] = { NULL };

	int   argc = 0;
	char *argv[255] = { NULL };

	int   r = 0;

	while (1)
	{
		buf[0] = '\0';

		r = shell_get_cmdline(buf, 255);
		if (r == EOF)
		{
			printf("exit\n");
			break;
		}

		if (!strcmp(buf, ""))
		{
			for (j = sh->first_j; j; j = j->next)
				job_wait(j, 1);

			continue;
		}

		cmdc = cmd_parse_cmdline(buf, cmds);
		if (cmdc == 0)
		{
			fprintf(stderr, "cmd_parse_cmdline: failed\n");
			continue;
		}

		j = job_new(sh->pgid, &sh->def_tmodes, &sh->tmodes);
		j->builtin_cmd_cb = shell_builtin_cmd;

		shell_push_job(j);

		for (int i = 0; i < cmdc; i++)
		{
			argc = cmd_parse_cmd(cmds[i], argv);

			cmd = cmd_new(0, argc, argv);
			if (builtin_cmd_exists(cmd))
				cmd->type = CMD_INTERNAL;

			job_push_cmd(j, cmd);

			shell_free_tmp_argv(argc, argv);
		}

		shell_free_tmp_argv(cmdc, cmds);

		job_launch(j, 1);
		shell_cleanup();
	}

	return 0;
}

