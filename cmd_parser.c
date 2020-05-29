#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cmd_parser.h"
#include "cmd_parser_lib.h"
#include "builtin_cmd.h"

CmdParser_t *cmd_parser_new()
{
	CmdParser_t *cps = calloc(1, sizeof(CmdParser_t));
	cps->cp_lib = cpl_get_lib();
	cps->flags |= CP_GIVE_INPUT;

	return cps;
}

int cmd_parser_status(CmdParser_t *cps)
{
	return cps->flags;
}

int cmd_parser_buf_alloc(CmdParser_t *cps, char *str)
{
	if (cps->buffer)
		return -1;

	cps->buffer = calloc(strlen(str) + 1, 1);
	memcpy(cps->buffer, str, strlen(str));

	cmd_parser_trim(cps->buffer);

	cps->bufsize = strlen(cps->buffer) + 1;

	return 0;
}

void cmd_parser_buf_free(CmdParser_t *cps)
{
	if (!cps->buffer)
		return;

	free(cps->buffer);
	cps->buffer = NULL;
}

int cmd_parser_feed(CmdParser_t *cps, char *str)
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

int cmd_parser_next_expr(CmdParser_t *cps)
{
	char c;
	int len = 0;

	while (1)
	{
		c = cps->buffer[cps->i];
		cmd_parser(cps, c);

		switch (cps->state)
		{
			case CPS_NOTHING:
				len += cps->cp_lib->cpl_nothing_cb(cps);
				break;

			case CPS_IN_ARG:
				len += cps->cp_lib->cpl_in_arg_cb(cps);
				break;

			case CPS_IN_DQSTRING:
				len += cps->cp_lib->cpl_in_dqstring_cb(cps);
				break;

			case CPS_IN_SQSTRING:
				len += cps->cp_lib->cpl_in_sqstring_cb(cps);
				break;

			case CPS_IN_CMD:
				len += cps->cp_lib->cpl_in_cmd_cb(cps);
				break;

			case CPS_CMDCHAIN_DONE:
				len += cps->cp_lib->cpl_cmdchain_done_cb(cps);
				break;

			default:
				fprintf(stderr, "default, state: %d\n", cps->state);
				break;
		}

		if (cps->state == CPS_DQSTRING_DONE)
		{
			fprintf(stderr, "dqstring done, at -%c-, len %d\n", cps->buffer[cps->i], len);
			break;
		}

		if (cps->state == CPS_SQSTRING_DONE)
		{
			break;
		}

		if (cps->state == CPS_ARG_DONE)
		{
			fprintf(stderr, "arg done\n");
			break;
		}

		if (cps->state == CPS_CMD_DONE)
		{
			fprintf(stderr, "cmd done\n");
			break;
		}

		if (cps->state == CPS_CMDCHAIN_DONE)
		{
			fprintf(stderr, "cmdchain done\n");
			break;
		}

		if (cps->state == CPS_PARSER_DONE)
		{
			fprintf(stderr, "parser done\n");
			break;
		}

		if (cps->state == CPS_PARSER_ERROR)
		{
			fprintf(stderr, "parser error\n");
			exit(0);
			break;
		}

		cps->i++;
	}

	return len;
}

void cmd_parser_skip_ws(CmdParser_t *cps)
{
	while (cmd_parser_c_is_ws(cps->buffer[cps->i]))
		cps->i++;
}

int cmd_parser_analyze(CmdParser_t *cps)
{
	char current_arg[255] = {'\0'};
	Cmd_t **current_cmd;

	if (!cps->buffer)
	{
		fprintf(stderr, "buf is not ready\n");
		return -1;
	}

	if (cps->flags & CP_GIVE_INPUT)
	{
		fprintf(stderr, "INPUT FLAG IS ON\n");
		return 1;
	}

	cps->output = cmd_new();
	current_cmd = &cps->output;

	int i_start;
	int expr_len = 0;

	while (1)
	{
		if ((*current_cmd)->ready)
		{
			current_cmd = &((*current_cmd)->next);
			*current_cmd = cmd_new();
		}

		cmd_parser_skip_ws(cps);

		i_start = cps->i;
		expr_len = cmd_parser_next_expr(cps);

		switch (cps->state)
		{
			case CPS_ARG_DONE:
				memcpy(current_arg, cps->buffer + i_start, expr_len);
				current_arg[expr_len] = '\0';
				cmd_add_arg(*current_cmd, current_arg);

				cps->state = CPS_IN_CMD;
				break;

			case CPS_DQSTRING_DONE:
			case CPS_SQSTRING_DONE:
				memcpy(current_arg, cps->buffer + i_start + 1, expr_len - 2);
				current_arg[expr_len - 2] = '\0';
				cmd_add_arg(*current_cmd, current_arg);

				cps->state = CPS_IN_CMD;
				break;

			case CPS_CMD_DONE:
				cmd_set_ready(*current_cmd);

				cps->state = CPS_NOTHING;
				break;

			case CPS_CMDCHAIN_DONE:
				cmd_set_ready(*current_cmd);
				if (cps->last_c_type == CT_DOUBLEPIPE)
				{
				}

				cps->flags |= CP_TAKE_OUTPUT;
				break;

			case CPS_PARSER_DONE:
				cps->flags |= CP_TAKE_OUTPUT;
				cps->flags |= CP_GIVE_INPUT;

				cps->state = CPS_NOTHING;
				break;

			default:
				break;
		}

		if (cps->flags & CP_TAKE_OUTPUT)
			break;

		if (cps->state == CPS_PARSER_ERROR)
			break;
	}

	if ((*current_cmd)->argc == 0)
	{
		free(*current_cmd);
		*current_cmd = NULL;
	}

	return cps->flags;
}

Cmd_t *cmd_parser_get_cmds(CmdParser_t *cps)
{
	cps->flags = 0;

	return cps->output;
}

void cmd_parser_output_free(CmdParser_t *cps)
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

void cmd_parser_reset(CmdParser_t* cps)
{
	cps->last_c_type = CT_NONE;
	cps->last_c_count = 0;
	cps->i = 0;
	cps->state = CPS_NOTHING;
	cps->flags = 0;
}

void cmd_parser_inc_last_type(CmdParser_t *cps, CmdParserCType_t ctype)
{
	if (cps->last_c_type == ctype)
	{
		cps->last_c_count++;
	}
	else
	{
		cps->last_c_type = ctype;
		cps->last_c_count = 1;
	}

	if (cps->last_c_type == CT_PIPE && cps->last_c_count == 1)
	{
		if (cps->buffer[cps->i + 1] == '|')
		{
			cps->last_c_type = CT_DOUBLEPIPE;
			cps->last_c_count = 1;
		}
	}
}

int cmd_parser(CmdParser_t* cps, char c)
{
	if (!c)
	{
		cps->last_c_type = CT_NULL;
		cps->last_c_count = 1;

		return 0;
	}

	if (cps->esc_next)
	{
		cmd_parser_inc_last_type(cps, CT_NORMAL);
		return 1;
	}

	if (c == '"')
	{

		cmd_parser_inc_last_type(cps, CT_NORMAL);

		if (cps->in_string && cps->dquote)
		{
			cps->dquote = 0;
			cps->in_string = 0;
			cmd_parser_inc_last_type(cps, CT_DOUBLEQUOTE);

			return 1;
		}

		if (!cps->in_string)
		{
			cps->dquote = 1;
			cps->in_string = 1;
			cmd_parser_inc_last_type(cps, CT_DOUBLEQUOTE);

			return 1;
		}

		return 1;
	}

	if (c == '\'')
	{
		cmd_parser_inc_last_type(cps, CT_NORMAL);

		if (cps->in_string && cps->squote)
		{
			cps->squote = 0;
			cps->in_string = 0;
			cmd_parser_inc_last_type(cps, CT_SINGLEQUOTE);

			return 1;
		}

		if (!cps->in_string)
		{
			cps->squote = 1;
			cps->in_string = 1;
			cmd_parser_inc_last_type(cps, CT_SINGLEQUOTE);

			return 1;
		}

		return 1;
	}

	if (cps->in_string)
	{
		cmd_parser_inc_last_type(cps, CT_NORMAL);
		return 1;
	}

	if (c == '\n')
	{
		cmd_parser_inc_last_type(cps, CT_NEWLINE);
		return 1;
	}

	if (c == '|')
	{
		cmd_parser_inc_last_type(cps, CT_PIPE);
		return 1;
	}

	if (c == ' ' || c == '\t')
	{
		cmd_parser_inc_last_type(cps, CT_WS);
		return 1;
	}

	if (c == ';')
	{
		cmd_parser_inc_last_type(cps, CT_SEMICOLON);
		return 1;
	}

	if (c == '\\')
	{
		cps->esc_next = 1;
		cmd_parser_inc_last_type(cps, CT_NORMAL);
		return 1;
	}

	cmd_parser_inc_last_type(cps, CT_NORMAL);

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


