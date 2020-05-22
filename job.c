#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "job.h"
#include "process.h"
#include "cmd.h"
#include "internal_cmds.h"

Job_t *job_new(pid_t sh_pgid, struct termios *tmodes, struct termios *sh_tmodes)
{
	Job_t *j = calloc(1, sizeof(Job_t));

	j->fd_stdin  = STDIN_FILENO;
	j->fd_stdout = STDOUT_FILENO;
	j->fd_stderr = STDERR_FILENO;
	j->shell_pgid = sh_pgid;
	j->sh_tmodes = sh_tmodes;
	j->interactive = 1;

	memcpy(&j->tmodes, tmodes, sizeof(struct termios));
	memcpy(j->sh_tmodes, sh_tmodes, sizeof(struct termios));

	return j;
}

void job_free(Job_t *j)
{
	Process_t* p;
	Process_t *tmp;
	Cmd_t *c;
	Cmd_t *tmp_c;

	p = j->first_p;
	while (p)
	{
		tmp = p->next;
		process_free(p);
		p = tmp;
	}

	c = j->first_c;
	while (c)
	{
		tmp_c = c->next;
		cmd_free(c);
		c = tmp_c;
	}

	free(j);
}

Process_t *job_push_process(Job_t *j, int argc, char **argv)
{
	Process_t **ptr;

	ptr = &j->first_p;
	while (*ptr)
		ptr = &((*ptr)->next);

	*ptr = process_new(argc, argv);
	return *ptr;
}

void job_push_cmd(Job_t *j, Cmd_t *c)
{
	Cmd_t **ptr;

	ptr = &j->first_c;
	while (*ptr)
		ptr = &((*ptr)->next);

	*ptr = c;
}

int job_is_stopped(Job_t* j)
{
	Process_t *p;
	for (p = j->first_p; p; p = p->next)
	{
		if (p->stopped)
			return 1;
	}

	return 0;
}

int job_is_completed(Job_t* j)
{
	Process_t *p;
	for (p = j->first_p; p; p = p->next)
	{
		if (!p->completed)
			return 0;
	}

	return 1;
}

int job_wait(Job_t* j)
{
	int status;
	int r;
	pid_t pid;

	while (1)
	{
		pid = waitpid(WAIT_ANY, &status, WUNTRACED);
		if (pid == 0 || errno == ECHILD) 
		{
			fprintf(stderr, "ECHILD\n");
			return -1;
		}

		if (pid < 0)
		{
			perror("waitpid");
			return -1;
		}

		r = job_update_status(j, pid, status);
		if (r)
			return r;

		if (job_is_stopped(j) || job_is_completed(j))
			break;
	}

	return 0;
}

int job_update_status(Job_t *j, pid_t pid, int status)
{
	Process_t *p;

	if (pid <= 0)
	{
		fprintf(stderr, "job_update_status: invalid pid %d\n", pid);
		return -1;
	}

	/* Update the record for the process.  */
	for (p = j->first_p; p; p = p->next)
	{
		if (p->pid == pid)
		{
			if (WIFSTOPPED(status))
			{
				fprintf (stderr, "child process %d is stopped\n", p->pid);
				p->stopped = 1;
			}
			else
			{
				fprintf (stderr, "child process %d is completed\n", p->pid);
				p->completed = 1;

				if (WIFSIGNALED(status))
					fprintf (stderr, "%d: Terminated by signal %d.\n",
							pid, WTERMSIG(status));
			}

			return 0;
		}
	}

	fprintf(stderr, "job_update_status: pid %d was not found\n", pid);
	return -1;
}

void job_put_fg(Job_t *j, int cont)
{
	tcsetpgrp(j->fd_stdin, j->pgid);

	if (cont)
	{
		tcsetattr(j->fd_stdin, TCSADRAIN, &j->tmodes);
		if (kill(-1 * j->pgid, SIGCONT) < 0)
		{
			perror("kill");
		}
	}

	(void)job_wait(j);

	tcgetattr(j->fd_stdin, &j->tmodes);
	tcsetattr(j->fd_stdin, TCSADRAIN, j->sh_tmodes);
	tcsetpgrp(j->fd_stdin, j->shell_pgid);
}

void job_put_bg(Job_t *j, int cont)
{
	if (cont)
	{
		if (kill(-1 * j->pgid, SIGCONT) < 0)
		{
			perror("kill");
		}
	}
}

void job_launch(Job_t* j, int foreground)
{
	Process_t *p;
	Cmd_t *c;
	pid_t pid;
	int p_pipe[2];
	int p_in;
	int p_out;
	int r;

	p_in = j->fd_stdin;

#if 1
	for (c = j->first_c; c; c = c->next)
	{
		p_out = j->fd_stdout;
		if (c->next)
		{
			if (pipe(p_pipe) < 0)
			{
				perror ("pipe");
				exit (1);
			}

			p_out = p_pipe[1];
		}

		p = job_push_process(j, c->argc, c->argv);

		p->fd_in  = p_in;
		p->fd_out = p_out;
		p->fd_err = j->fd_stderr;

		if (c->type == CMD_EXECUTABLE)
		{
			pid = process_fork(p, j->pgid, j->fd_stdin, j->interactive, foreground);

			p->pid = pid;
			if (j->interactive)
			{
				if (!j->pgid)
					j->pgid = pid;

				setpgid(pid, j->pgid);
			}
		}
		else if (c->type == CMD_INTERNAL)
		{
			r = j->internal_cmd_cb(p->argv[0], p->fd_in, p->fd_out, p->fd_err);
			if (r < 0)
			{
				fprintf(stderr, "internal_call failed\n");
			}
		}

		if (p_in != j->fd_stdin)
			close(p_in);
		if (p_out != j->fd_stdout)
			close(p_out);

		p_in = p_pipe[0];
	}

#endif

#if 0
	for (p = j->first_p; p; p = p->next)
	{
		p_out = j->fd_stdout;
		if (p->next)
		{
			if (pipe(p_pipe) < 0)
			{
				perror ("pipe");
				exit (1);
			}

			p_out = p_pipe[1];
		}

		p->fd_in  = p_in;
		p->fd_out = p_out;
		p->fd_err = j->fd_stderr;

		pid = fork();
		if (pid < 0)
		{
			perror ("fork");
			exit(1);
		}

		if (pid == 0) // child 
			process_launch(p, j->pgid, j->fd_stdin, j->interactive, foreground);

		// parent continues from here
		p->pid = pid; // pid is child's pid 
		if (j->interactive)
		{
			if (!j->pgid)
				j->pgid = pid;

			setpgid(pid, j->pgid);
		}

		// The parent won't need the pipes. 
		if (p_in != j->fd_stdin)
			close(p_in);
		if (p_out != j->fd_stdout)
			close(p_out);

		p_in = p_pipe[0]; // input fd for the next process in the list 
	}
#endif

	if (!isatty(j->fd_stdin))
	{
		(void)job_wait(j);
		return;
	}	

	if (foreground)
		job_put_fg(j, 0);
	else
		job_put_bg(j, 0);
}

