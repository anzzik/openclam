#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "cmd.h"

typedef struct CmdParserState_s CmdParserState_t;
struct CmdParserState_s
{
	int in_string;
	int esc_next;
	int c_is_pipe;
	int c_is_null;
	int c_is_semicolon;

	int dquote;
	int squote;
};

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

int cmd_is_ws(char c)
{
	if (c == ' ' || c == '\n' || c == '\t')
		return 1;

	return 0;
}

void cmd_trim_begin(char *str)
{
	int ws_c = 0;
	char c;
       
	c = str[0];
	while (c && cmd_is_ws(c))
		c = str[++ws_c];

	memmove(str, str + ws_c, strlen(str) + 1);
}

void cmd_trim_end(char *str)
{
	int l;
	char c;
       
	l = strlen(str);

	c = str[--l];
	while (cmd_is_ws(c))
		c = str[--l];

	str[l + 1] = '\0';
}

void cmd_trim(char *str)
{
	cmd_trim_begin(str);
	cmd_trim_end(str);
}

int cmd_parser(CmdParserState_t* cps, char c)
{
	if (!c)
	{
		cps->c_is_null = 1;
		return 0;
	}

	if (cps->esc_next)
	{
		cps->esc_next = 0;
		return 1;
	}

	if (c == '\\')
	{
		cps->esc_next = 1;
		return 1;
	}

	if (c == ';')
	{
		cps->c_is_semicolon = 1;
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
	}

	if (c == '\'')
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
	}

	if (!cps->in_string && c == '|')
		cps->c_is_pipe = 1;

	return 1;
}

int cmd_parse_cmdline(char *cmdline, char **cmds)
{
	int i = 0;
	int r = 0;
	int c_i = 0;
	int cmd_i = 0;
	char cmd[255] = {'\0'};
	char c;

	CmdParserState_t parser;
	memset(&parser, '\0', sizeof(CmdParserState_t));

	do
	{
		c = cmdline[i++];
		r = cmd_parser(&parser, c);

		if (parser.c_is_null)
		{
			if (parser.in_string)
				fprintf(stderr, "Missing an ending quote\n");

			cmd[c_i] = '\0';
			c_i = 0;

			goto l_cmd_ready;
		}

		if (parser.c_is_pipe)
		{
			parser.c_is_pipe = 0;
			cmd[c_i] = '\0';
			c_i = 0;

			goto l_cmd_ready;
		}

		cmd[c_i++] = c;

		continue;

l_cmd_ready:
		cmd_trim(cmd);
		cmds[cmd_i] = malloc(strlen(cmd) + 1);
		memcpy(cmds[cmd_i], cmd, strlen(cmd) + 1);
		cmd_i++;

	} while (!parser.c_is_null);

	return cmd_i;
}

int cmd_parse_cmd(char *cmd, char **argv)
{
	char *tok;
	int argv_i = 0;

	tok = strtok(cmd, " \t\"\'");
	if (!tok)
	{
		argv[0] = malloc(strlen(cmd) + 1);
		memcpy(argv[0], cmd, strlen(cmd) + 1);

		return 1;
	}

	while (tok)
	{
		argv[argv_i] = malloc(strlen(tok) + 1);
		memcpy(argv[argv_i], tok, strlen(tok) + 1);

		argv_i++;
		tok = strtok(NULL, " \t\"\'");
	}

	return argv_i;
}


