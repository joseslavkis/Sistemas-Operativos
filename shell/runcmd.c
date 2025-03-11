#include "runcmd.h"

int status = 0;
struct cmd *parsed_pipe;

void handle_background_process(struct cmd *parsed, pid_t p);
void handle_foreground_process(struct cmd *parsed, pid_t p);


void
handle_background_process(struct cmd *parsed, pid_t p)
{
	print_back_info(parsed);
	waitpid(p, &status, WNOHANG);
}

void
handle_foreground_process(struct cmd *parsed, pid_t p)
{
	waitpid(p, &status, 0);
	print_status_info(parsed);
}

// runs the command in 'cmd'
int
run_cmd(char *cmd)
{
	pid_t p;
	struct cmd *parsed;

	// if the "enter" key is pressed
	// just print the prompt again
	if (cmd[0] == END_STRING)
		return 0;

	// "history" built-in call
	if (history(cmd))
		return 0;

	// "cd" built-in call
	if (cd(cmd))
		return 0;

	// "exit" built-in call
	if (exit_shell(cmd))
		return EXIT_SHELL;

	// "pwd" built-in call
	if (pwd(cmd))
		return 0;

	// parses the command line
	parsed = parse_line(cmd);

	// forks and run the command
	if ((p = fork()) == 0) {
		// keep a reference
		// to the parsed pipe cmd
		// so it can be freed later
		if (parsed->type == PIPE)
			parsed_pipe = parsed;

		exec_cmd(parsed);
	}

	// stores the pid of the process
	parsed->pid = p;

	// background process special treatment
	// Hint:
	// - check if the process is
	//		going to be run in the 'back'
	// - print info about it with
	// 	'print_back_info()'
	//
	// Your code here

	// waits for the process to finish
	if (parsed->type == BACK) {
		handle_background_process(parsed, p);
	} else {
		handle_foreground_process(parsed, p);
	}


	free_command(parsed);

	return 0;
}
