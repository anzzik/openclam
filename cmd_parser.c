#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cmd_parser.h"
#include "builtin_cmd.h"

CmdParserState_t *cmd_parser_new()
{
	CmdParserState_t *cps = calloc(1, sizeof(CmdParserState_t));

	cps->flags |= CPS_GIVE_INPUT;

	return cps;
}

int cmd_parser_status(CmdParserState_t *cps)
{
	return cps->flags;
}

int cmd_parser_buf_alloc(CmdParserState_t *cps, char *str)
{
	if (cps->buf_is_alloced)
		return -1;

	cps->buffer = calloc(strlen(str) + 1, 1);
	memcpy(cps->buffer, str, strlen(str));

	cmd_parser_trim(cps->buffer);

	cps->bufsize = strlen(cps->buffer) + 1;
	cps->buf_is_alloced = 1;

	return 0;
}

void cmd_parser_buf_free(CmdParserState_t *cps)
{
	if (!cps->buf_is_alloced)
		return;

	free(cps->buffer);
	cps->buf_is_alloced = 0;
	cps->buffer = NULL;
}

int cmd_parser_feed(CmdParserState_t *cps, char *str)
{
	cmd_parser_buf_free(cps);
	cmd_parser_buf_alloc(cps, str);

	cps->flags = 0;

	return 0;
}

int cmd_parser_c_is_ws(char c)
{
	if (c == ' ' || c == '\t' || c == '\n')
		return 1;

	return 0;
}

char cmd_parser_next_c(CmdParserState_t *cps)
{
	char c;

	if (cps->c_ws)
	{
		while (cmd_parser_c_is_ws(cps->buffer[cps->i]))
			cps->i++;
	}

	c = cps->buffer[cps->i++];
	cmd_parser(cps, c);

	return c;
}

int cmd_parser_analyze(CmdParserState_t *cps)
{
	char current_arg[255] = {'\0'};
	char c;
	int  arg_i = 0;
	Cmd_t **current_cmd;

	if (!cps->buffer)
	{
		fprintf(stderr, "buf is not ready\n");
		return -1;
	}

	if (cps->flags & CPS_GIVE_INPUT)
	{
		fprintf(stderr, "INPUT FLAG IS ON\n");
		return 1;
	}

	cps->output = cmd_new();
	current_cmd = &cps->output;

	while (1)
	{
		c = cmd_parser_next_c(cps);

		if ((*current_cmd)->ready)
		{
			current_cmd = &((*current_cmd)->next);
			*current_cmd = cmd_new();
		}

		if (cps->c_ws)
		{
			if (arg_i == 0)
				continue;

			current_arg[arg_i] = '\0';
			cmd_add_arg(*current_cmd, current_arg);
			arg_i = 0;

			continue;
		}

		if (cps->c_newline)
		{
			if (arg_i == 0)
				continue;

			current_arg[arg_i] = '\0';

			cmd_add_arg(*current_cmd, current_arg);
			cmd_set_ready(*current_cmd);
			arg_i = 0;

			cps->flags = CPS_TAKE_OUTPUT;

			break;
		}

		if (cps->c_null)
		{
			if (arg_i > 0)
			{
				current_arg[arg_i] = '\0';

				cmd_add_arg(*current_cmd, current_arg);
				arg_i = 0;
			}

			cmd_set_ready(*current_cmd);
			cps->flags = CPS_TAKE_OUTPUT | CPS_GIVE_INPUT;

			break;
		}

		if (cps->c_pipe)
		{
			current_arg[arg_i] = '\0';

			if (arg_i > 0)
			{
				cmd_add_arg(*current_cmd, current_arg);
				arg_i = 0;
			}

			cmd_set_ready(*current_cmd);

			continue;
		}

		if (cps->c_semicolon)
		{
			current_arg[arg_i] = '\0';
			if (arg_i > 0)
			{
				cmd_add_arg(*current_cmd, current_arg);
				arg_i = 0;
			}

			cmd_set_ready(*current_cmd);

			cps->flags = CPS_TAKE_OUTPUT;

			break;
		}

		current_arg[arg_i++] = c;
	}

	if ((*current_cmd)->argc == 0)
	{
		cmd_free(*current_cmd);
		*current_cmd = NULL;
	}

	return cps->flags;
}

Cmd_t *cmd_parser_get_cmds(CmdParserState_t *cps)
{
	cps->flags = 0;

	return cps->output;
}

void cmd_parser_output_free(CmdParserState_t *cps)
{
	Cmd_t **ptr = &(cps->output);
	Cmd_t *tmp;

	while (*ptr)
	{
		tmp = (*ptr)->next;
		cmd_free(*ptr);
		*ptr = tmp;
	}
}

void cmd_parser_reset(CmdParserState_t* cps)
{
	memset(cps, '\0', sizeof(CmdParserState_t));
}

int cmd_parser(CmdParserState_t* cps, char c)
{
	if (!c)
	{
		cps->c_null = 1;
		return 0;
	}

	if (cps->esc_next)
	{
		cps->esc_next = 0;
		return 1;
	}

	if (c == '"')
	{
		if (cps->dquote)
		{
			cps->dquote = 0;
			cps->in_string = 0;
		}
		else if (!cps->in_string)
		{
			cps->dquote = 1;
			cps->in_string = 1;
		}

		cps->c_semicolon = 0;
		cps->c_ws = 0;
		cps->c_pipe = 0;
		cps->c_newline = 0;

	}
	else if (c == '\'')
	{
		if (cps->squote)
		{
			cps->squote = 0;
			cps->in_string = 0;
		}
		else if (!cps->in_string)
		{
			cps->squote = 1;
			cps->in_string = 1;
		}

		cps->c_semicolon = 0;
		cps->c_ws = 0;
		cps->c_pipe = 0;
		cps->c_newline = 0;
	}
	else if (!cps->in_string && c == '\n')
	{
		cps->c_newline++;

		cps->c_pipe = 0;
		cps->c_ws = 0;
		cps->c_semicolon = 0;
	}
	else if (!cps->in_string && c == '|')
	{
		cps->c_pipe++;

		cps->c_newline = 0;
		cps->c_ws = 0;
		cps->c_semicolon = 0;
	}
	else if (!cps->in_string && (c == ' ' || c == '\t'))
	{
		cps->c_ws++;

		cps->c_pipe = 0;
		cps->c_newline = 0;
		cps->c_semicolon = 0;
	}
	else if (!cps->in_string && c == ';')
	{
		cps->c_semicolon++;

		cps->c_ws = 0;
		cps->c_pipe = 0;
		cps->c_newline = 0;
	}
	else if (!cps->in_string && c == '\\')
	{
		cps->esc_next = 1;

		cps->c_semicolon = 0;
		cps->c_ws = 0;
		cps->c_pipe = 0;
		cps->c_newline = 0;
	}
	else
	{
		cps->c_semicolon = 0;
		cps->c_ws = 0;
		cps->c_pipe = 0;
		cps->c_newline = 0;
	}

	return 1;
}

void cmd_parser_trim_begin(char *str)
{
	int ws_c = 0;
	char c;
       
	c = str[0];
	while (c && cmd_parser_c_is_ws(c))
		c = str[++ws_c];

	memmove(str, str + ws_c, strlen(str) + 1);
}

void cmd_parser_trim_end(char *str)
{
	int l;
	char c;
       
	l = strlen(str);

	c = str[--l];
	while (cmd_parser_c_is_ws(c))
		c = str[--l];

	str[l + 1] = '\0';
}

void cmd_parser_trim(char *str)
{
	cmd_parser_trim_begin(str);
	cmd_parser_trim_end(str);
}

Cmd_t *cmd_new()
{
	Cmd_t *c = calloc(1, sizeof(Cmd_t));

	c->type = CMD_EXECUTABLE;

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

