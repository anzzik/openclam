#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "shell.h"
#include "cmd.h"
#include "internal_cmds.h"

Shell_t *sh;

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

int shell_internal_cmd(Cmd_t *c, int fd_in, int fd_out, int fd_err)
{
	(void)internal_call(c, sh, fd_in, fd_out, fd_err);

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

int shell_parse_cmd(char *cmd, char **argv)
{
	char *tok;
	int argv_i = 0;

	tok = strtok(cmd, " \t\"\'");
	if (!tok)
	{
		argv[0] = malloc(strlen(cmd) + 1);
		memcpy(argv[0], cmd, strlen(cmd) + 1);

		return 1;
	}

	while (tok)
	{
		argv[argv_i] = malloc(strlen(tok) + 1);
		memcpy(argv[argv_i], tok, strlen(tok) + 1);

		argv_i++;
		tok = strtok(NULL, " \t\"\'");
	}

	return argv_i;
}

int shell_tok_parse_cmdline(char *cmdline, char **cmds)
{
	char *tok;
	int cmd_i = 0;

	tok = strtok(cmdline, "|");
	if (!tok)
	{
		cmds[0] = cmdline;
		return 1;
	}

	while (tok)
	{
		cmds[cmd_i++] = tok;
		tok = strtok(NULL, "|");
	}

	return cmd_i;
}

int shell_parse_cmdline(char* cmdline, char **cmds)
{
	int i = 0;
	char c;
	int c_i = 0;
	int cmd_i = 0;
	int quote = 0;
	int d_quote = 0;
	int esc = 0;
	char cmd[255] = {'\0'};

	do
	{
		c = cmdline[i++];

		if (!esc && c == '"')
		{
			if (d_quote)
				d_quote = 0;
			else if (!d_quote && quote)
				d_quote = 0;
			else if (!d_quote && !quote)
				d_quote = 1;
		}

		if (!esc && c == '\'')
		{
			if (quote)
				quote = 0;
			else if (!quote && d_quote)
				quote = 0;
			else if (!quote && !d_quote)
				quote = 1;
		}

		if (c == '|' && !quote && !d_quote && !esc)
		{
			cmd[c_i] = '\0';
			c_i = 0;

			goto l_cmd_ready;
		}

		cmd[c_i++] = c;

		if (esc)
			esc = 0;

		if (!quote && !d_quote && !esc && c == '\\')
			esc = 1;

		if (!c)
		{
			if (quote || d_quote)
			{
				fprintf(stderr, "Missing a quote\n");
				return -1;
			}

			goto l_cmd_ready;
		}

		continue;

l_cmd_ready:
		cmds[cmd_i] = malloc(strlen(cmd) + 1);
		memcpy(cmds[cmd_i], cmd, strlen(cmd) + 1);
		cmd_i++;


	} while (c);

	return cmd_i;
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
			{
				job_wait(j, 1);
			}
			continue;
		}

		cmdc = shell_parse_cmdline(buf, cmds);
		if (cmdc <= 0)
		{
			shell_free_tmp_argv(cmdc, cmds);
			continue;
		}

		j = job_new(sh->pgid, &sh->def_tmodes, &sh->tmodes);
		j->internal_cmd_cb = shell_internal_cmd;

		shell_push_job(j);

		for (int i = 0; i < cmdc; i++)
		{
			argc = shell_parse_cmd(cmds[i], argv);

			cmd = cmd_new(0, argc, argv);
			if (internal_cmd_exists(cmd))
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

