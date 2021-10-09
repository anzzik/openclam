#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"

Token_t *token_develop_list(Token_t *head)
{
	enum State {
		NORMAL = 0,
		IN_SQ_STR,
		IN_DQ_STR,
		IN_STR,
		AFTER_AND,
	};

	enum State s = NORMAL;
	Token_t *t = head;
	Token_t *prev = 0;
	Token_t *multipart = 0;
	Token_t *new_head = head;
	char buf[255] = { 0 };
	int buf_at = 0;

	while (t)
	{
		if (t->type == TT_SQ || t->type == TT_DQ)
		{
			if (s == NORMAL)
			{
				buf_at = 0;
				memcpy(buf + buf_at++, t->buffer, t->n + 1);

				multipart = t;
				s = t->type == TT_SQ ? IN_SQ_STR : IN_DQ_STR;

				goto l_next;
			}

			if (s == IN_SQ_STR || s == IN_DQ_STR)
			{
				TType_t type = s == IN_SQ_STR ? TT_SQ_STR : TT_DQ_STR;

				memcpy(buf + buf_at, t->buffer, t->n + 1);
				token_set_buffer(multipart, buf, type);

				Token_t *it = multipart->next;
				while (it != t->next)
				{
					token_free(it);
					it = it->next;
				}

				multipart->next = t->next;
				s = NORMAL;

				goto l_next;
			}
		}

		if (s == IN_SQ_STR || s == IN_DQ_STR)
		{
			memcpy(buf + buf_at, t->buffer, t->n);
			buf_at += t->n;

			goto l_next;
		}

		if (t->type == TT_PIPE)
		{
		       if (prev->type == TT_PIPE)
		       {
			       token_set_buffer(prev, "||", TT_OR);
			       prev->next = t->next;
			       token_free(t);
			       goto l_next;
			}
		}

		if (t->type == TT_EQUALS)
		{
			if (prev->type == TT_EQUALS)
			{
				token_set_buffer(prev, "==", TT_DOUBLEEQUALS);
				prev->next = t->next;
				token_free(t);
				goto l_next;
			}
		}

l_next:
		prev = t;
		t = t->next;
	}

	return new_head;
}

Token_t *token_develop_assignments(Token_t *head)
{
	enum State {
		NORMAL = 0,
		IN_ASSIGNMENT,
		SYNTAX_ERROR
	};

	Token_t *new_head = head;
	Token_t *t = head;
	Token_t *prev = 0;
	enum State state = NORMAL;

	while (t)
	{
		if (state == IN_ASSIGNMENT)
		{
			t->type = TT_VARVALUE;
			state = NORMAL;
			goto l_next;
		}

		if (t->type == TT_EQUALS)
		{
			if (!prev || prev->type != TT_WORD)
			{
				fprintf(stderr, "Error in assignment, bad var name\n");
				state = SYNTAX_ERROR;
				break;
			}

			if (!t->next || (t->next->type != TT_WORD && t->next->type != TT_NUMBER))
			{
				fprintf(stderr, "Error in assignment\n");
				state = SYNTAX_ERROR;
				break;
			}

			prev->type = TT_VARDECL;
			state = IN_ASSIGNMENT;
			goto l_next;
		}

l_next:
		prev = t;
		t = t->next;
	}

	if (state == SYNTAX_ERROR)
		new_head = 0;

	return new_head;
}

void token_free(Token_t *t)
{
	if (t->n)
		free(t->buffer);

	free(t);
}

TType_t token_get_type(const char *str)
{
	if (!strlen(str))
		return TT_EOL;

	if (!strcmp(str, " ") || !strcmp(str, "\t"))
		return TT_WS;

	if (!strcmp(str, "\n"))
		return TT_NEWLINE;

	if (!strcmp(str, "\""))
		return TT_DQ;

	if (!strcmp(str, "\\"))
		return TT_ESC;

	if (!strcmp(str, "'"))
		return TT_SQ;

	if (!strcmp(str, "|"))
		return TT_PIPE;

	if (!strcmp(str, ";"))
		return TT_SEMICOLON;

	if (!strcmp(str, "{"))
		return TT_OPEN_CURLY;

	if (!strcmp(str, "}"))
		return TT_CLOSE_CURLY;

	if (!strcmp(str, "&&"))
		return TT_AND;

	if (!strcmp(str, "||"))
		return TT_OR;

	if (!strcmp(str, "="))
		return TT_EQUALS;

	if (!strcmp(str, "=="))
		return TT_DOUBLEEQUALS;

	for (int i = 0; *(str+i); i++)
		if (str[i] < 48 || str[i] > 57)
			return TT_WORD;

	return TT_NUMBER;
}

int token_is_c_token(char c)
{
	switch (c)
	{
		case '|':
		case ';':
		case '{':
		case '}':
		case ' ':
		case '"': 
		case '=':
		case '\'':
		case '\\':
		case '\0':
		case '\t':
		case '\n':
			return 1;

		default:
			return 0;
	}
}

int token_is_cmd_end_type(TType_t type)
{
	switch (type)
	{
		case TT_SEMICOLON:
		case TT_OR:
		case TT_AND:
		case TT_PIPE:
		case TT_EOL:
			return 1;

		default:
			return 0;
	}
}

Token_t *token_new(char *buf)
{
	Token_t *t = calloc(1, sizeof(Token_t));
	token_set_buffer(t, buf, TT_UNKNOWN);

	return t;
}

void token_print_list(Token_t *head)
{
	Token_t *t = head;
	char eol_s[] = "(eol)";
	char *s;

	while (t)
	{
		s = t->buffer;
		if (t->type == TT_EOL)
			s = eol_s;

		printf("token %2d: '%s'\n", t->type, s);
		t = t->next;
	}
}

Token_t *token_process_str(char *buf)
{
	char c;
	int token_start_at = -1;
	int token_len = 0;

	char t_buf[255] = { 0 };
	int at = 0;

	Token_t *t_first = 0;

	while (1)
	{
		c = buf[at++];

		if (token_start_at < 0)
		{
			/* the first char is a token itself */
			if (token_is_c_token(c))
			{
				t_buf[0] = c;
				t_buf[1] = '\0';
				t_first = token_push(t_first, token_new(t_buf));
				continue;
			}

			token_start_at = at - 1;
			continue;
		}

		/* we are parsing a word token and come across a token char */
		if (token_start_at >= 0 && token_is_c_token(c))
		{
			token_len = at - token_start_at - 1;
			memcpy(t_buf, buf + token_start_at, token_len);
			t_buf[token_len] = '\0';
			t_first = token_push(t_first, token_new(t_buf));

			t_buf[0] = c;
			t_buf[1] = '\0';

			t_first = token_push(t_first, token_new(t_buf));
			token_start_at = -1;
		}

		if (!c)
			break;
	}

	t_first = token_develop_list(t_first);
	if (!t_first)
		return 0;

	t_first = token_develop_assignments(t_first);
	if (!t_first)
		return 0;

	t_first = token_trim_list(t_first);
	if (!t_first)
		return 0;

	return t_first;
}

Token_t *token_push(Token_t *head, Token_t *t)
{
	if (!head)
		return t;

	Token_t *c = head;
	while (c->next)
		c = c->next;

	c->next = t;
	return head;
}

Token_t *token_trim_list(Token_t *head)
{
	Token_t *c = head;
	Token_t *new_head = head;
	Token_t *prev = 0;

	while (c)
	{
		if (c->type == TT_WS)
		{
			if (!prev)
			{
				new_head = c->next;
				token_free(c);
				goto l_next;
			}

			prev->next = c->next;
			token_free(c);
			goto l_next;
		}

		prev = c;
l_next:
		c = c->next;
	}

	return new_head;
}

void token_set_buffer(Token_t* t, char *buf, TType_t type)
{
	if (t->n)
	{
		free(t->buffer);
		t->n = 0;
	}

	if (!strlen(buf))
	{
		t->n = 0;
		t->type = TT_EOL; 
		return;
	}

	t->buffer = calloc(1, strlen(buf));
	t->n = strlen(buf);
	memcpy(t->buffer, buf, strlen(buf) + 1);

	t->type = type;
	if (t->type == TT_UNKNOWN)
		t->type = token_get_type(t->buffer);
}

