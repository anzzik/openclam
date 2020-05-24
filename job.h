#ifndef JOB_H
#define JOB_H

#include "process.h"
#include "cmd.h"

typedef struct Job_s Job_t;
struct Job_s
{
	pid_t pgid;
	pid_t shell_pgid;

	int fd_stdin;
	int fd_stdout;
	int fd_stderr;

	int interactive;
	int bg;
	struct termios tmodes;
	struct termios *sh_tmodes;

	int (*builtin_cmd_cb)(Cmd_t *c, int, int, int);

	Process_t *first_p;
	Cmd_t *first_c;

	Job_t *next;
};

Job_t *job_new(pid_t sh_pgid, struct termios *tmodes, struct termios *sh_tmodes);
void job_free(Job_t *j);
Process_t *job_push_process(Job_t *j, int argc, char **argv);
void job_push_cmd(Job_t *j, Cmd_t *c);
void job_launch(Job_t* j, int foreground);
int job_is_stopped(Job_t* j);
int job_is_completed(Job_t* j);
int job_is_bg(Job_t *j);
void job_put_fg(Job_t *j, int cont);
void job_put_bg(Job_t *j, int cont);
int job_continue(Job_t *j);
int job_wait(Job_t* j, int nonblock);
int job_update_status(Job_t *j, pid_t pid, int status);

#endif

