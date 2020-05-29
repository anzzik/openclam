#include <stdlib.h>
#include <string.h>

#include "cmd.h"
#include "builtin_cmd.h"

Cmd_t *cmd_new()
{
	Cmd_t *c = calloc(1, sizeof(Cmd_t));

	c->type = CMD_EXECUTABLE;

	return c;
}

Cmd_t *cmd_copy(Cmd_t *c_src)
{
	Cmd_t *current_src;
	Cmd_t **current_dst;
	Cmd_t *c;
	int i;

	current_dst = &c;
	current_src = c_src;

	while (current_src)
	{
		*current_dst = cmd_new();

		(*current_dst)->type = current_src->type;
		(*current_dst)->argc = current_src->argc;

		for (i = 0; i < current_src->argc; i++)
		{
			(*current_dst)->argv[i] = calloc(strlen(current_src->argv[i]) + 1, 1);
			memcpy((*current_dst)->argv[i], current_src->argv[i], strlen(current_src->argv[i]));
		}

		current_src = current_src->next;
		current_dst = &((*current_dst)->next);
	}

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

void cmd_set_ready(Cmd_t *c)
{
	c->ready = 1;

	if (!c->argc)
		return;

	if (builtin_cmd_exists(c))
		c->type = CMD_INTERNAL;
}

void cmd_push(Cmd_t *c, Cmd_t *new_c)
{
	Cmd_t **ptr;

	ptr = &(c->next);

	while (*ptr)
		ptr = &((*ptr)->next);

	*ptr = new_c;
}

