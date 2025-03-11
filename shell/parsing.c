#include "parsing.h"

extern int status;

// parses an argument of the command stream input
static char *
get_token(char *buf, int idx)
{
	char *tok;
	int i;

	tok = (char *) calloc(ARGSIZE, sizeof(char));
	i = 0;

	while (buf[idx] != SPACE && buf[idx] != END_STRING) {
		tok[i] = buf[idx];
		i++;
		idx++;
	}

	return tok;
}

// parses and changes stdin/out/err if needed
static bool
parse_redir_flow(struct execcmd *c,
                 char *arg)  // arg: Cadena que contiene el posible simbolo de redireccion (>, >>, <).
{                            // >: establece la redireccion para stdout o stderr
	int inIdx, outIdx;  // <: establece la redireccion para stdin

	// flow redirection for output
	if ((outIdx = block_contains(arg, '>')) >= 0) {
		switch (outIdx) {
		// stdout redir
		case 0: {
			strcpy(c->out_file, arg + 1);
			break;
		}
		// stderr redir
		case 1: {
			strcpy(c->err_file, &arg[outIdx + 1]);
			break;
		}
		}
		free(arg);
		c->type = REDIR;

		return true;
	}

	// flow redirection for input
	if ((inIdx = block_contains(arg, '<')) >= 0) {
		// stdin redir
		strcpy(c->in_file, arg + 1);

		c->type = REDIR;
		free(arg);

		return true;
	}

	return false;
}

// parses and sets a pair KEY=VALUE
// environment variable
static bool
parse_environ_var(struct execcmd *c, char *arg)
{
	// sets environment variables apart from the
	// ones defined in the global variable "environ"
	if (block_contains(arg, '=') > 0) {
		// checks if the KEY part of the pair
		// does not contain a '-' char which means
		// that it is not a environ var, but also
		// an argument of the program to be executed
		// (For example:
		// 	./prog -arg=value
		// 	./prog --arg=value
		// )
		if (block_contains(arg, '-') < 0) {
			c->eargv[c->eargc++] = arg;
			return true;
		}
	}
	return false;
}

// this function will be called for every token, and it should
// expand environment variables. In other words, if the token
// happens to start with '$', the correct substitution with the
// environment value should be performed. Otherwise the same
// token is returned. If the variable does not exist, an empty string should be
// returned within the token
//
// Hints:
// - check if the first byte of the argument contains the '$'
// - expand it and copy the value in 'arg'
// - remember to check the size of variable's value
//		It could be greater than the current size of 'arg'
//		If that's the case, you should realloc 'arg' to the new size.
static char *
expand_environ_var(char *arg)
{
	if (arg[0] == '$') {
		if (strncmp(arg, "$?", 2) == 0) {  // $? status
			extern int status;
			int status_len = snprintf(
			        NULL,
			        0,
			        "%d",
			        status);  // status: es una variable global que
			                  // contiene el estado de salida del ultimo comando ejecutado
			arg = realloc(
			        arg,
			        status_len +
			                1);  // cambia el tamanio de la memoria
			                     // asignada a arg sumandole 1 por el '\0'
			snprintf(arg,
			         status_len + 1,
			         "%d",
			         status);  // copia el valor de status en arg
			return arg;
		} else {
			char *name = arg + 1;
			char *value = getenv(name);

			// Si la variable de entorno existe
			if (value) {
				int value_len = strlen(value);
				arg = realloc(arg,
				              value_len +
				                      1);  // Realoca el tamanio de arg si es necesario
				strcpy(arg,
				       value);  // Copia el valor de la variable de entorno en arg
			} else {
				// Si la variable no existe, libero el espacio y devuelvo cadena vacia.
				free(arg);
				arg = strdup("");
			}
		}
	}
	return arg;
}
// parses one single command having into account:
// - the arguments passed to the program
// - stdin/stdout/stderr flow changes
// - environment variables (expand and set)
static struct cmd *
parse_exec(char *buf_cmd)
{
	struct execcmd *c;
	char *tok;
	int idx = 0, argc = 0;

	c = (struct execcmd *) exec_cmd_create(buf_cmd);

	while (buf_cmd[idx] != END_STRING) {
		tok = get_token(buf_cmd, idx);
		idx = idx + strlen(tok);

		if (buf_cmd[idx] != END_STRING)
			idx++;

		if (parse_redir_flow(c, tok))
			continue;

		if (parse_environ_var(c, tok))
			continue;

		tok = expand_environ_var(tok);

		if (strlen(tok) > 0) {
			// Si el token no está vacio, lo agrego.
			c->argv[argc++] = tok;
		}
	}

	c->argv[argc] = (char *) NULL;
	c->argc = argc;

	return (struct cmd *) c;
}

// parses a command knowing that it contains the '&' char
static struct cmd *
parse_back(char *buf_cmd)
{
	int i = 0;
	struct cmd *e;

	while (buf_cmd[i] != '&')
		i++;

	buf_cmd[i] = END_STRING;

	e = parse_exec(buf_cmd);

	return back_cmd_create(e);
}

// parses a command and checks if it contains the '&'
// (background process) character
static struct cmd *
parse_cmd(char *buf_cmd)
{
	if (strlen(buf_cmd) == 0)
		return NULL;

	int idx;

	// checks if the background symbol is after
	// a redir symbol, in which case
	// it does not have to run in in the 'back'
	if ((idx = block_contains(buf_cmd, '&')) >= 0 && buf_cmd[idx - 1] != '>')
		return parse_back(buf_cmd);

	return parse_exec(buf_cmd);
}
// parses the command line
// looking for the pipe character '|'
struct cmd *
parse_line(char *buf)
{
	struct cmd *cmd1, *cmd2;
	char *pipe_pos = strchr(buf, '|');

	if (pipe_pos != NULL) {
		*pipe_pos = END_STRING;
		cmd1 = parse_cmd(buf);
		cmd2 = parse_line(pipe_pos + 1);
	} else {
		cmd1 = parse_cmd(buf);
		cmd2 = NULL;
	}

	if (cmd2 != NULL) {
		return pipe_cmd_create(cmd1, cmd2);
	} else {
		return cmd1;
	}
}