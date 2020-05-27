#ifndef CMD_PARSER_H
#define CMD_PARSER_H

#define CPS_GIVE_INPUT 1
#define CPS_TAKE_OUTPUT 2

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
	CT_SEMICOLON,
	CT_WS,
	CT_NEWLINE,
	CT_PIPE,
	CT_DOUBLEPIPE,
	CT_NORMAL
};

typedef struct CmdParserState_s CmdParserState_t;
struct CmdParserState_s
{
	int in_string;
	int esc_next;
	
	CmdParserCType_t last_c_type;
	int last_c_count;

	int dquote;
	int squote;

	char *buffer;
	int  bufsize;
	int  i;

	Cmd_t *output;

	int flags;
};

CmdParserState_t *cmd_parser_new();
int cmd_parser_buf_alloc(CmdParserState_t *cps, char *str);
void cmd_parser_buf_free(CmdParserState_t *cps);
int cmd_parser_status(CmdParserState_t *cps);
int cmd_parser_feed(CmdParserState_t *cps, char *str);
int cmd_parser_analyze(CmdParserState_t *cps);
int cmd_parser_get_cmd(CmdParserState_t *cps);
char cmd_parser_next_c(CmdParserState_t *cps);
void cmd_parser_inc_last_type(CmdParserState_t *cps, CmdParserCType_t ctype);
int cmd_parser(CmdParserState_t* cps, char c);
Cmd_t *cmd_parser_get_cmds(CmdParserState_t *cps);
void cmd_parser_output_free(CmdParserState_t *cps);
void cmd_parser_reset(CmdParserState_t* cps);
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

