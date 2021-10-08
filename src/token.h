#ifndef TOKEN_H
#define TOKEN_H

typedef enum TType_s TType_t;
enum TType_s
{
	TT_UNKNOWN = 0,
	TT_EOL,
	TT_WS,
	TT_NEWLINE,
	TT_DQ,
	TT_SQ,
	TT_ESC,
	TT_PIPE,
	TT_OPEN_CURLY,
	TT_CLOSE_CURLY,
	TT_SEMICOLON,
	TT_AND,
	TT_WORD,

	/* round 2 tokens */
	TT_DQ_STR,
	TT_SQ_STR,
	TT_OR,
};

typedef struct Token_s Token_t;
struct Token_s
{
	TType_t type;
	char *buffer;
	size_t n;
	Token_t *next;
};


void token_develop_list(Token_t *head);
void token_free(Token_t *t);
TType_t token_get_type(const char *str);
int token_is_c_token(char c);
int token_is_cmd_end_type(TType_t type);
Token_t *token_new(char *buf);
void token_print_list(Token_t *head);
Token_t *token_process_str(char *buf);
Token_t *token_push(Token_t *head, Token_t *t);
void token_set_buffer(Token_t* t, char *buf, TType_t type);
Token_t *token_trim_list(Token_t *head);

#endif

