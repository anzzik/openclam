#include <stdio.h>

#include "cmd_parser.h"

static int cpl_on_cps_nothing(void *parser);
static int cpl_on_cps_in_arg(void* parser);
static int cpl_on_cps_in_dqstring(void* parser);
static int cpl_on_cps_in_sqstring(void* parser);
static int cpl_on_cps_in_cmd(void* parser);
static int cpl_on_cps_cmdchain_done(void* parser);

static CPLib_t cp_lib = {
	.cpl_nothing_cb = cpl_on_cps_nothing,
	.cpl_in_arg_cb = cpl_on_cps_in_arg,
	.cpl_in_dqstring_cb = cpl_on_cps_in_dqstring,
	.cpl_in_sqstring_cb = cpl_on_cps_in_sqstring,
	.cpl_in_cmd_cb = cpl_on_cps_in_cmd,
	.cpl_cmdchain_done_cb = cpl_on_cps_cmdchain_done
};

CPLib_t *cpl_get_lib()
{
	return &cp_lib;
}

static int cpl_on_cps_nothing(void *parser)
{
	int r = 0;
	CmdParser_t *cps = parser;
	
	switch (cps->last_c_type)
	{
		case CT_WS:
		case CT_NEWLINE:
			break;

		case CT_PIPE:
			cps->i += 1;
			cps->state = CPS_CMD_DONE;
			break;

		case CT_DOUBLEPIPE:
			cps->i += 2;
			cps->state = CPS_CMDCHAIN_DONE;
			break;

		case CT_SEMICOLON:
			break;

		case CT_NULL:
			cps->state = CPS_CMDCHAIN_DONE;
			break;

		case CT_DOUBLEQUOTE:
			r = 1;
			cps->state = CPS_IN_DQSTRING;
			break;

		case CT_SINGLEQUOTE:
			r = 1;
			cps->state = CPS_IN_SQSTRING;
			break;

		case CT_NORMAL:
			r = 1;
			cps->state = CPS_IN_ARG;
			break;

		default:
			fprintf(stderr, "on_cps_nothing: CPS_PARSER_ERROR (c_type: %d)\n", cps->last_c_type);
			cps->state = CPS_PARSER_ERROR;
	}

	return r;
}

static int cpl_on_cps_in_arg(void* parser)
{
	CmdParser_t *cps = parser;
	int r = 0;

	switch (cps->last_c_type)
	{
		case CT_WS:
		case CT_PIPE:
		case CT_DOUBLEPIPE:
		case CT_NEWLINE:
		case CT_NULL:
			cps->state = CPS_ARG_DONE;
			break;

		case CT_SEMICOLON:
			cps->state = CPS_ARG_DONE;
			break;

		case CT_NORMAL:
			r = 1;
			break;

		default:
			fprintf(stderr, "on_cps_in_arg: CPS_PARSER_ERROR (c_type: %d)\n", cps->last_c_type);
			cps->state = CPS_PARSER_ERROR;
	}

	return r;
}

static int cpl_on_cps_in_dqstring(void* parser)
{
	CmdParser_t *cps = parser;
	int r = 1;

	if (cps->last_c_type == CT_DOUBLEQUOTE)
	{
		cps->i++;
		cps->state = CPS_DQSTRING_DONE;
	}

	return r;
}

static int cpl_on_cps_in_sqstring(void* parser)
{
	CmdParser_t *cps = parser;
	int r = 1;

	if (cps->last_c_type == CT_SINGLEQUOTE)
	{
		cps->i++;
		cps->state = CPS_SQSTRING_DONE;
	}

	return r;
}

static int cpl_on_cps_in_cmd(void* parser)
{
	CmdParser_t *cps = parser;
	int r = 0;

	switch (cps->last_c_type)
	{
		case CT_WS:
			break;

		case CT_SEMICOLON:
			cps->i++;
			cps->state = CPS_CMDCHAIN_DONE;
			break;

		case CT_NEWLINE:
		case CT_NULL:
			cps->state = CPS_CMDCHAIN_DONE;
			break;

		case CT_DOUBLEPIPE:
			cps->i += 2;
			cps->state = CPS_CMDCHAIN_DONE;
			break;

		case CT_PIPE:
			cps->i += 1;
			cps->state = CPS_CMD_DONE;
			break;

		case CT_DOUBLEQUOTE:
			r = 1;
			cps->state = CPS_IN_DQSTRING;
			break;

		case CT_SINGLEQUOTE:
			r = 1;
			cps->state = CPS_IN_SQSTRING;
			break;

		case CT_NORMAL:
			r = 1;
			cps->state = CPS_IN_ARG;
			break;

		default:
			fprintf(stderr, "on_cps_in_cmd: CPS_PARSER_ERROR (c_type: %d)\n", cps->last_c_type);
			cps->state = CPS_PARSER_ERROR;
	}

	return r;
}

static int cpl_on_cps_cmdchain_done(void* parser)
{
	CmdParser_t *cps = parser;
	int r = 0;

	switch (cps->last_c_type)
	{
		case CT_WS:
		case CT_NEWLINE:
			cps->state = CPS_NOTHING;
			break;

		case CT_NULL:
			cps->state = CPS_PARSER_DONE;
			break;

		case CT_NORMAL:
			r = 1;
			cps->state = CPS_IN_ARG;
			break;

		default:
			fprintf(stderr, "on_cps_cmdchain_done: CPS_PARSER_ERROR (c_type: %d)\n", cps->last_c_type);
			cps->state = CPS_PARSER_ERROR;
	}

	return r;
}

