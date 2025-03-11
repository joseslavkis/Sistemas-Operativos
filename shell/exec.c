#include "exec.h"

// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	char key[BUFLEN], value[BUFLEN];

	for (int i = 0; i < eargc; i++) {
		int idx = block_contains(eargv[i], '=');
		if (idx < 0) {
			continue;
		}
		get_environ_key(eargv[i], key);
		get_environ_value(eargv[i], value, idx);
		setenv(key, value, 1);
	}
}
// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int
open_redir_fd(char *file, int flags)
{
	int fd;
	if (flags & O_CREAT) {  // if o_creat esta entre los flags
		fd = open(file,
		          flags,
		          S_IRUSR | S_IWUSR);  // se abre con permisos escritura/lectura para user
	} else {
		fd = open(file, flags);  // se abre sin permisos extra
	}

	if (fd < 0) {
		perror("Error abriendo file");
	}

	return fd;
}
// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void
exec_cmd(struct cmd *cmd)
{
	// To be used in the different cases
	struct execcmd *e;
	struct backcmd *b;
	struct execcmd *r;
	struct pipecmd *p;

	switch (cmd->type) {
	case EXEC:
		e = (struct execcmd *) cmd;
		set_environ_vars(e->eargv, e->eargc);
		if (execvp(e->argv[0], e->argv) < 0) {
			perror("execvp");
			_exit(-1);
		}
		_exit(0);
		break;

	case BACK: {
		b = (struct backcmd *) cmd;
		pid_t pid = fork();
		if (pid == 0) {
			exec_cmd(b->c);
			_exit(0);
		}
		break;
	}

	case REDIR: {
		r = (struct execcmd *) cmd;
		if (strlen(r->out_file) > 0) {
			int fd_out = open_redir_fd(r->out_file,
			                           O_WRONLY | O_CREAT | O_TRUNC);
			if (fd_out < 0) {
				perror("open error");
				_exit(-1);
			}
			dup2(fd_out, STDOUT_FILENO);
			close(fd_out);
		}
		if (strlen(r->in_file) > 0) {
			int fd_in = open_redir_fd(r->in_file, O_RDONLY);
			if (fd_in < 0) {
				perror("open error");
				_exit(-1);
			}
			dup2(fd_in, STDIN_FILENO);
			close(fd_in);
		}
		if (strlen(r->err_file) > 0) {
			if (strcmp(r->err_file, "&1") == 0) {
				dup2(STDOUT_FILENO, STDERR_FILENO);
			} else {
				int fd_err = open_redir_fd(r->err_file,
				                           O_WRONLY | O_CREAT |
				                                   O_TRUNC);
				if (fd_err < 0) {
					perror("open error");
					_exit(-1);
				}
				dup2(fd_err, STDERR_FILENO);
				close(fd_err);
			}
		}
		r->type =
		        EXEC;  // cambio al tipo EXEC para evitar ciclos y que
		               // se ejecute (el comando llega a exec con r->type = REDIR, por eso lo cambio).
		exec_cmd((struct cmd *) r);  // llamo recursivamente
		break;
	}
	case PIPE: {
		p = (struct pipecmd *) cmd;
		int pipe_fds[2];

		if (pipe(pipe_fds) < 0) {
			perror("pipe error");
			_exit(-1);
		}

		pid_t pid1 = fork();
		if (pid1 < 0) {
			perror("fork error");
			_exit(-1);
		}

		if (pid1 == 0) {
			close(pipe_fds[0]);
			if (dup2(pipe_fds[1], STDOUT_FILENO) < 0) {
				perror("dup2 error");
				_exit(-1);
			}
			close(pipe_fds[1]);
			exec_cmd(p->leftcmd);
		}

		pid_t pid2 = fork();
		if (pid2 < 0) {
			perror("fork error");
			_exit(-1);
		}

		if (pid2 == 0) {
			close(pipe_fds[1]);
			if (dup2(pipe_fds[0], STDIN_FILENO) < 0) {
				perror("dup2 error");
				_exit(-1);
			}
			close(pipe_fds[0]);
			exec_cmd(p->rightcmd);
		}

		close(pipe_fds[0]);
		close(pipe_fds[1]);
		waitpid(pid1, NULL, 0);
		waitpid(pid2, NULL, 0);
		_exit(0);
	}
	}
}