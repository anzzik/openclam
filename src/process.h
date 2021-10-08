#ifndef PROCESS_H
#define PROCESS_H

typedef struct Process_s Process_t;
struct Process_s
{
	pid_t pid;

	int argc;
	char* argv[50];

	int fd_in;
	int fd_out;
	int fd_err;

	int stopped;
	int completed;
	int internal;

	Process_t *next;
};

Process_t *process_new(int argc, char **argv);
void process_free(Process_t *p);
void process_start_child(Process_t *p, pid_t pgid, int shell_fd, int interactive, int foreground);
int process_fork(Process_t *p, pid_t pgid, int shell_fd, int interactive, int foreground);

#endif

