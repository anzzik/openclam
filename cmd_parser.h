#ifndef CMD_PARSER_H
#define CMD_PARSER_H

#define CP_GIVE_INPUT 1
#define CP_TAKE_OUTPUT 2

typedef enum CmdType_e CmdType_t;
enum CmdType_e
{
	CMD_INTERNAL = 0,
	CMD_EXECUTABLE
};

typedef struct Cmd_s Cmd_t;
struct Cmd_s
{
	CmdType_t type;
	int   argc;
	char *argv[50];

	int ready;

	Cmd_t *next;
};

typedef enum CmdParserCType_e CmdParserCType_t;
enum CmdParserCType_e
{
	CT_NONE = 0,
	CT_NULL,
	CT_DOUBLEQUOTE,
	CT_SINGLEQUOTE,
	CT_SEMICOLON,
	CT_WS,
	CT_NEWLINE,
	CT_PIPE,
	CT_DOUBLEPIPE,
	CT_NORMAL
};

typedef enum CmdParserState_e CmdParserState_t;
enum CmdParserState_e
{
	CPS_NOTHING = 0,

	CPS_IN_ARG,
	CPS_ARG_DONE,

	CPS_IN_DQSTRING,
	CPS_IN_SQSTRING,

	CPS_DQSTRING_DONE,
	CPS_SQSTRING_DONE,

	CPS_CMD_DONE,
	CPS_CMDCHAIN_DONE,

	CPS_PARSER_DONE,
	CPS_PARSER_ERROR
};

typedef struct CmdParser_s CmdParser_t;
struct CmdParser_s
{
	int in_string;
	int esc_next;
	
	CmdParserCType_t last_c_type;
	int last_c_count;

	CmdParserState_t state;
	int dquote;
	int squote;

	char *buffer;
	int  bufsize;
	int  i;

	Cmd_t *output;

	int flags;
};

CmdParser_t *cmd_parser_new();
int cmd_parser_buf_alloc(CmdParser_t *cps, char *str);
void cmd_parser_buf_free(CmdParser_t *cps);
int cmd_parser_status(CmdParser_t *cps);
int cmd_parser_feed(CmdParser_t *cps, char *str);
int cmd_parser_analyze(CmdParser_t *cps);
int cmd_parser_next_expr(CmdParser_t *cps);
int cmd_parser_get_cmd(CmdParser_t *cps);
void cmd_parser_inc_last_type(CmdParser_t *cps, CmdParserCType_t ctype);
void cmd_parser_skip_ws(CmdParser_t *cps);
int cmd_parser(CmdParser_t* cps, char c);
Cmd_t *cmd_parser_get_cmds(CmdParser_t *cps);
void cmd_parser_output_free(CmdParser_t *cps);
void cmd_parser_reset(CmdParser_t* cps);
void cmd_parser_trim_begin(char *str);
void cmd_parser_trim_end(char *str);
void cmd_parser_trim(char *str);

Cmd_t *cmd_new();
Cmd_t *cmd_copy(Cmd_t *c_src);
void cmd_free(Cmd_t *c);
void cmd_add_arg(Cmd_t *c, char *arg);
void cmd_set_ready(Cmd_t *c);
void cmd_push(Cmd_t *c, Cmd_t *new_c);

#endif

