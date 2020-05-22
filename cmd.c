#include <string.h>
#include <stdlib.h>
#include "cmd.h"

Cmd_t *cmd_new(int internal, int argc, char  **argv)
{
	Cmd_t *c = calloc(1, sizeof(Cmd_t));

	c->argc = argc;
	for (int i = 0; i < c->argc; i++)
	{
		c->argv[i] = calloc(strlen(argv[i]) + 1, 1);
		memcpy(c->argv[i], argv[i], strlen(argv[i]));
	}

	c->type = CMD_EXECUTABLE;
	if (internal)
		c->type = CMD_INTERNAL;

	return c;
}

void cmd_free(Cmd_t *c)
{
	for (int i = 0; i < c->argc; i++)
		free(c->argv[i]);

	free(c);
}

void cmd_add_arg(Cmd_t *c, char *arg)
{
	c->argv[c->argc] = calloc(strlen(arg) + 1, 1);
	memcpy(c->argv[c->argc], arg, strlen(arg));
	c->argc++;
}

void cmd_push_next(Cmd_t *c, Cmd_t *new_c)
{
	Cmd_t **ptr;

	ptr = &(c->next);

	while (*ptr)
		ptr = &((*ptr)->next);

	*ptr = new_c;
}

