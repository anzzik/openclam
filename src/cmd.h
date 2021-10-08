#ifndef CMD_H
#define CMD_H

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
	int   ready;
	int   piped;

	Cmd_t *next;
};

Cmd_t *cmd_new();
Cmd_t *cmd_copy(Cmd_t *c_src);
void cmd_free(Cmd_t *c);
void cmd_add_arg(Cmd_t *c, char *arg);
void cmd_set_ready(Cmd_t *c);
void cmd_print(Cmd_t *c);
void cmd_push(Cmd_t *c_head, Cmd_t *c);


#endif

