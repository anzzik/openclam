#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cmd_parser.h"
#include "builtin_cmd.h"

CmdParser_t *cmd_parser_new()
{
	CmdParser_t *cps = calloc(1, sizeof(CmdParser_t));

	return cps;
}

void cmd_parser_set_token_list(CmdParser_t* cmdp, Token_t *list_head)
{
	cmdp->t_head = list_head;
	cmdp->t_current = list_head;
}

void cmd_parser_free_token_list(CmdParser_t* cmdp)
{
	Token_t *t = cmdp->t_head;
	while (t)
	{
		token_free(t);
		t = t->next;
	}
}

void cmd_parser_advance(CmdParser_t *cmdp)
{
	if (cmdp->t_current->type == TT_EOL)
	{
		fprintf(stderr, "advancing from EOL!\n");
		return;
	}

	cmdp->t_current = cmdp->t_current->next;
}

Cmd_t *cmd_parser_next_cmd(CmdParser_t *cmdp)
{
	Cmd_t *cmd = cmd_new();
	Token_t *t;
	char argv0[255] = { 0 };

	if (!cmdp->t_current)
	{
		fprintf(stderr, "t_current is 0\n");
		return 0;
	}

	if (cmdp->t_current->type == TT_EOL)
	{
		cmdp->state = CPS_PARSER_DONE;
		return 0;
	}

	while (!token_is_cmd_end_type(cmdp->t_current->type))
	{
		t = cmdp->t_current;
		if (argv0[0] == '\0')
		{
			if (t->type != TT_WORD)
			{
				cmdp->state = CPS_PARSER_ERROR;
				return 0;
			}

			memcpy(argv0, t->buffer, t->n);
			cmd_add_arg(cmd, argv0);
			goto l_next;
		}

		cmd_add_arg(cmd, t->buffer);
l_next:
		cmd_parser_advance(cmdp);
	}

	if (cmdp->t_current->type == TT_PIPE)
		cmd->piped = 1;

	if (cmdp->t_current->type != TT_EOL)
		cmd_parser_advance(cmdp);

	cmd_set_ready(cmd);

	return cmd;
}

Cmd_t *cmd_parser_next_cmdgrp(CmdParser_t *cmdp)
{
	Cmd_t *cmd, *c_cmd;

	cmd = cmd_parser_next_cmd(cmdp);
	if (!cmd)
		return 0;

	c_cmd = cmd;
	while (c_cmd && c_cmd->piped)
	{
		fprintf(stderr, "adding a piped cmd\n");
		c_cmd = cmd_parser_next_cmd(cmdp);
		if (c_cmd)
			cmd_push(cmd, c_cmd);
	}

	return cmd;
}

