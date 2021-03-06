#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include <sys/types.h>
#include <termios.h>

#include "shell.h"
#include "cmd_parser.h"
#include "builtin_cmd.h"
#include "token.h"

static Shell_t *sh;

int shell_init()
{
	pid_t fg_pgid;

	sh        = malloc(sizeof(Shell_t));
	sh->fd    = STDIN_FILENO;
	sh->pid   = getpid();
	sh->pgid  = getpgrp();

	if (!isatty(sh->fd))
	{
		fprintf(stderr, "%s\n", "shell is NOT interactive");
		return -1;
	}

	/* get foreground process group id */
	fg_pgid = tcgetpgrp(sh->fd);
	while (fg_pgid != sh->pgid)
	{
		fprintf(stderr, "shell is not in foreground, have to stop it until fg..\n");

		/* this puts the process group in T state and blocks until woken up by fg */
		kill(-1 * sh->pgid, SIGTTIN); // 
		fg_pgid = tcgetpgrp(sh->fd);
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

	/* put the shell process group on foreground */
	tcsetpgrp(sh->fd, sh->pid);

	/* get termios attributes from the pty device (fd) */
	tcgetattr(sh->fd, &sh->tmodes);
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
	shell_free_jobs(0);
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
	char *cwd;

	cwd = getcwd(NULL, 0);

	printf("%s > ", cwd);
	fflush(stdout);
	free(cwd);

	sprintf(fmt_str, "%%%d[^\n]", n - 1);

	read_c = scanf(fmt_str, buf);
	getchar();

	return read_c;
}

void shell_free_jobs(int only_completed)
{
	Job_t **ptr;
	Job_t *tmp;

	ptr = &sh->first_j;
	while (*ptr)
	{
		if (!only_completed)
			goto l_do_free;

		if (job_is_completed(*ptr))
			goto l_do_free;

		ptr = &((*ptr)->next);
		continue;

l_do_free:
		tmp = *ptr;
		*ptr = (*ptr)->next;
		job_free(tmp);
	}
}

void shell_nb_wait()
{
	Job_t *j;

	for (j = sh->first_j; j; j = j->next)
		job_wait(j, 1);
}

int shell_mainloop()
{
	Cmd_t *cmd;
	CmdParser_t *cmdp = cmd_parser_new();
	int retval = 0;

	while (1)
	{
		char cmdline_buf[255] = { '\0' };
		int r = shell_get_cmdline(cmdline_buf, 255);
		if (r == EOF)
		{
			printf("exit\n");
			break;
		}

		if (!strcmp(cmdline_buf, ""))
		{
			shell_nb_wait();
			shell_free_jobs(1);
			continue;
		}

		Token_t *t_list = token_process_str(cmdline_buf);
		if (!t_list)
		{
			retval = -1;
			break;
		}

		cmd_parser_set_token_list(cmdp, t_list);
		while ((cmd = cmd_parser_next_cmdgrp(cmdp)))
		{
			Job_t *j = job_new(sh->pgid, &sh->def_tmodes, &sh->tmodes);

			j->builtin_cmd_cb = shell_builtin_cmd;
			shell_push_job(j);
			job_set_cmd(j, cmd);
			job_launch(j, 1);

			shell_free_jobs(1);
		}

		cmd_parser_free_token_list(cmdp);
	}

	return retval;
}

