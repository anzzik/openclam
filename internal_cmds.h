#ifndef INTERNAL_CMDS_H
#define INTERNAL_CMDS_H

#include "cmd.h"

typedef struct InternalCmdDef_s InternalCmdDef_t;
struct InternalCmdDef_s
{
	char *cmd_name;
	int (*cmd_fn)(Cmd_t*,void*,int,int,int);
};

int internal_call(Cmd_t *c, void *u_ptr, int fd_in, int fd_out, int fd_err);
int internal_cmd_exists(Cmd_t *c);

#endif

