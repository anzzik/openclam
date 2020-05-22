#ifndef SHELL_H
#define SHELL_H

#include <sys/types.h>
#include <termios.h>

#include "job.h"


typedef struct Shell_s Shell_t;
struct Shell_s
{
	Job_t *first_j;
	int shell_fd;
	pid_t pid;
	pid_t pgid;
	pid_t fg_pgid;
	int interactive;
	struct termios tmodes;
	struct termios def_tmodes;
};

int shell_init();
void shell_free();
int shell_get_cmdline();
void shell_free_tmp_argv(int argc, char **argv);
int shell_parse_cmd(char *cmd, char **argv);
int shell_tok_parse_cmdline(char *cmdline, char **argv);
int shell_parse_cmdline(char *cmdline, char **argv);
int shell_internal_cmd(const char *name, int fd_in, int fd_out, int fd_err);
int shell_mainloop();

#endif

