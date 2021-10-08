#ifndef CMD_PARSER_H
#define CMD_PARSER_H

#include "cmd.h"
#include "token.h"

typedef enum CmdParserState_e CmdParserState_t;
enum CmdParserState_e
{
	CPS_PARSER_DONE,
	CPS_PARSER_ERROR
};


typedef struct CmdParser_s CmdParser_t;
struct CmdParser_s
{
	Token_t *t_current;
	Token_t *t_head;
	CmdParserState_t state;
};

void        cmd_parser_free_token_list(CmdParser_t* cmdp);
CmdParser_t *cmd_parser_new();
Cmd_t       *cmd_parser_next_cmd(CmdParser_t *cmdp);
Cmd_t       *cmd_parser_next_cmdgrp(CmdParser_t *cmdp);
void        cmd_parser_set_token_list(CmdParser_t* cmdp, Token_t *list_head);

#endif

