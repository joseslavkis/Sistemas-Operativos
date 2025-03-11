#include "builtin.h"


// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	if (strcmp(cmd, "exit") == 0) {
		return 1;
	}
	return 0;
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']

int
cd(char *cmd)
{
	if (strncmp(cmd, "cd", 2) != 0 ||
	    (cmd[2] != SPACE && cmd[2] != END_STRING)) {
		return 0;
	}
	char *directory = split_line(cmd, SPACE);

	if (!directory || !*directory) {      // si directorio no es valido
		char *home = getenv("HOME");  // obtiene el env de home
		if (home == NULL) {
			return 0;
		}
		if (chdir(home) < 0) {
			perror("Error al cambiar al directorio home");
			return 0;
		}
		snprintf(prompt, sizeof(prompt), "(%s)", home);
		return 1;
	}

	if (directory[0] == '$') {
		directory++;
		char *env = getenv(directory);
		if (env && *env != ' ') {
			directory = env;
		} else {
			return 0;
		}
	}

	if (chdir(directory) < 0) {
		perror("Error al cambiar al directorio");
		return 0;
	}
	char buffer[BUFLEN];
	if (getcwd(buffer, sizeof(buffer)) == NULL) {  // para obtener ruta actual
		return 0;
	}

	snprintf(prompt, sizeof(prompt), "(%s)", buffer);
	return 1;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	if (strcmp(cmd, "pwd") != 0) {
		return 0;
	}
	char dir[BUFLEN];

	if (getcwd(dir, sizeof(dir)) == NULL) {
		perror("Error al buscar la ruta del directorio actual");
		exit(EXIT_FAILURE);
	}
	printf("%s\n", dir);
	return 1;
}


// returns true if `history` was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
history(char *cmd)
{
	// Your code here

	return 0;
}