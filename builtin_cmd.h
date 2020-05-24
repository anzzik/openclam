#ifndef BUILTIN_CMDS_H
#define BUILTIN_CMDS_H

#include "cmd.h"

int builtin_call(Cmd_t *c, void *u_ptr, int fd_in, int fd_out, int fd_err);
int builtin_cmd_exists(Cmd_t *c);

#endif

