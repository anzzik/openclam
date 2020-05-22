#ifndef INTERNAL_CMDS_H
#define INTERNAL_CMDS_H

typedef struct InternalCmdDef_s InternalCmdDef_t;
struct InternalCmdDef_s
{
	char *cmd_name;
	int (*cmd_fn)(void*,int,int,int);
};

int internal_call(const char* cmd_name, void *u_arg, int fd_in, int fd_out, int fd_err);

#endif

