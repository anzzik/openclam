#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "process.h"

Process_t *process_new(int argc, char **argv)
{
	Process_t *p = calloc(1, sizeof(Process_t));

	p->argc = argc;
	for (int i = 0; i < p->argc; i++)
	{
		p->argv[i] = malloc(strlen(argv[i]) + 1);
		memcpy(p->argv[i], argv[i], strlen(argv[i]) +1);
	}

	p->argv[p->argc] = NULL;

	return p;
}

void process_free(Process_t *p)
{
	for (int i = 0; i < p->argc; i++)
		free(p->argv[i]);

	free(p);
}

int process_fork(Process_t *p, pid_t pgid, int shell_fd, int interactive, int foreground)
{
	pid_t pid;

	pid = fork();
	if (pid < 0)
	{
		perror ("fork");
		exit(1);
	}

	if (pid == 0) 
		process_start_child(p, pgid, shell_fd, interactive, foreground);

	return pid;
}

void process_start_child(Process_t *p, pid_t pgid, int shell_fd, int interactive, int foreground)
{
	if (interactive)
	{
		/* Put the process into the process group and give the process group
		   the terminal, if appropriate.
		   This has to be done both by the shell and in the individual
		   child processes because of potential race conditions.  */

		p->pid = getpid();
		if (pgid == 0) 
			pgid = p->pid;

		setpgid(p->pid, pgid);

		if (foreground && tcgetpgrp(shell_fd) != pgid)
			tcsetpgrp(shell_fd, pgid);

		signal(SIGINT,  SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		signal(SIGTSTP, SIG_DFL);
		signal(SIGTTIN, SIG_DFL);
		signal(SIGTTOU, SIG_DFL);
		signal(SIGCHLD, SIG_DFL);
	}

	/* Set the standard input/output channels of the new process.  */
	if (p->fd_in != STDIN_FILENO)
	{
		dup2(p->fd_in, STDIN_FILENO);
		close(p->fd_in);
	}

	if (p->fd_out != STDOUT_FILENO)
	{
		dup2(p->fd_out, STDOUT_FILENO);
		close(p->fd_out);
	}

	if (p->fd_err != STDERR_FILENO)
	{
		dup2(p->fd_err, STDERR_FILENO);
		close(p->fd_err);
	}

	/* Exec the new process.  Make sure we exit.  */
	execvp(p->argv[0], p->argv);
	perror("execvp");
	exit(1);
}

