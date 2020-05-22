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

	Cmd_t *next;
};

Cmd_t *cmd_new(int internal, int argc, char  **argv);
void cmd_free(Cmd_t *c);
void cmd_add_arg(Cmd_t *c, char *arg);
void cmd_push_next(Cmd_t *c, Cmd_t *new_c);

#endif

