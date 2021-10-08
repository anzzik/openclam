#ifndef SHELL_H
#define SHELL_H

#include "job.h"

typedef struct Shell_s Shell_t;
struct Shell_s
{
	int fd;

	pid_t pid;
	pid_t pgid;

	struct termios tmodes;
	struct termios def_tmodes;

	Job_t *first_j;
};

int shell_init();
void shell_free();
void shell_push_job(Job_t *j);
int shell_get_cmdline();
void shell_free_jobs(int only_completed);
int shell_builtin_cmd(Cmd_t *c, int fd_in, int fd_out, int fd_err);
void shell_nb_wait();
int shell_mainloop();

#endif

