// --------------------------- Headers --------------------------- //

#include <stdio.h>  // perror, printf
#include <stdlib.h>
#include <string.h>  // mem allocation
#include <fcntl.h>  // open
#include <unistd.h>  // fork, close, execv, getpid
#include <sys/types.h>  // pid_t
#include <sys/wait.h>  // waitpid
#include <signal.h>  // Signal handlers
#include "src/shell_info.h"  // shell info struct

// --------------------- Function Prototypes --------------------- //
void other_cmd(char* args[], struct shell_info *info);
void custom_SIGINT();
void custom_IG();

// Global variable declaration for custom Signal Handler
int stop_background;

// ---------------------- Helper Functions ----------------------- //

// --------------------------------------------------------------- //
// function   : void print_args(..)
// parameters : char* args[]
// description: Prints arguments stored in an array of char pointers
// --------------------------------------------------------------- //
void print_args(char* args[]) {
	int i = 0;
	while (args[i]) {
		printf("%s \n", args[i]);
		i++;
	}
}


// --------------------------------------------------------------- //
// function   : free_memory(..)
// parameters : char* line
// 						  char* args[]
// description: Frees dynamically allocated memory in parameters
// --------------------------------------------------------------- //
void free_memory(char* line, char* args[]) {
	free(line);
	line = NULL;

	int i = 0;
	while (args[i]) {
		free(args[i]);   // Free each call to malloc
		args[i] = NULL;  // Point to NULL
		i++;
	}
}


// ------------------- Built-In Shell Functions ------------------ //

// --------------------------------------------------------------- //
// function   : my_exit()
// parameters : None
// description: Ends the shell program when user enters 'exit' cmd
// --------------------------------------------------------------- //
int my_exit() {
	printf("exiting shell \n");

	// flush after every display to stdout to show all output
	fflush(stdout);

	return 0;  // terminates smallsh by ending do-while loop
}


// --------------------------------------------------------------- //
// function   : my_cd(..)
// parameters : char* args []
// description: Changes directory. If no argument is entered, makes
//              the current working directory (CWD) to environment
//              variable HOME. Otherwise, sets CWD to argument
// example    : cd Documents/Code-Projects/my_c_shell
// --------------------------------------------------------------- //
void my_cd(char* args[]) {
	if (!args[1]) {  // No arguments, set directory to HOME
		if (chdir(getenv("HOME")) != 0)
			printf("chdir() failed");  // Display in event of error

	} else {
		if (chdir(args[1]) != 0)
			printf("chdir() failed");
	}
}


// --------------------------------------------------------------- //
// function   : my_status(..)
// parameters : int exit_status
// description: Prints out either the exit status or the terminating
//              signal of the last foreground process ran by the shell
//              Decodes status by using termination MACROS
// --------------------------------------------------------------- //
void my_status(int exit_status) {
	if (WIFEXITED(exit_status)) {
		printf("exit value %d \n", WEXITSTATUS(exit_status));
		fflush(stdout);

	} else {
		printf("terminated by signal %d \n", WTERMSIG(exit_status));
		fflush(stdout);
	}
}


// ------------------ I/O Redirection Functions ------------------ //

// --------------------------------------------------------------- //
// function   : output_redirection(..)
// parameters : char* filename
// description: Redirects output from stdout to the passed in file
// reference  : Handling input/output redirection
//              https://stackoverflow.com/a/11518304/10895933
// --------------------------------------------------------------- //
void output_redirection(char* filename) {
	// Open file and set permissions
	int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0777);

	if (fd == -1) {  // Error checking
		perror("Output file could not be opened \n");  // Print error message
		exit(1);  // Set the exit status to 1
	}

	int d = dup2(fd, 1);  // Direct output to file descriptor

	if (d == -1) {
		perror("Output file could not be redirected");
		exit(1);
	}

	close(fd);  // Close file
}


// --------------------------------------------------------------- //
// function   : input_redirection(..)
// parameters : char* filename
// description: Redirects input from stdin to the passed in file
// reference  : Handling input/output redirection
//              https://stackoverflow.com/a/11518304/10895933
// --------------------------------------------------------------- //
void input_redirection(char* filename) {
	// Open file and set permissions
	int fd = open(filename, O_RDONLY);

	if (fd == -1) {  // Error checking
		perror("Input file could not be opened \n");  // Print error message
		exit(1);  // Set the exit status to 1
	}

	int d = dup2(fd, 0);  // Direct output to file descriptor

	if (d == -1) {
		perror("Input file could not be redirected");
		exit(0);
	}

	close(fd);  // Close file
}


// -------------------- User Input Functions --------------------- //

// --------------------------------------------------------------- //
// function   : get_input()
// parameters : none
// description: Gets input from user
//              Allocates memory
//              Returns pointer to user input
// references : How to use fgets - http://sekrit.de/webdocs/c/beginners-guide-away-from-scanf.html
//              remove newline from fgets - https://stackoverflow.com/a/2693826/10895933
// --------------------------------------------------------------- //
char* get_input() {
	char buf[2048];
	char* line = NULL;

	printf(": ");  // Prompt user
	fflush(stdout);

	fgets(buf, 2048, stdin);  // Get input from user

	strtok(buf, "\n");  // Remove newline from fgets

	line = malloc(sizeof(char) * sizeof(buf));  // Allocate memory to line

	strcpy(line, buf);  // Store input in line

	return line;  // return user input
}


// --------------------------------------------------------------- //
// function   : parse_line(..)
// parameters : char* line
//              struct shell_info *info
//              char* args[]
// description: Splits line param into tokens delimited by whitespace " "
//              Allocates memory for each token
//              Stores tokens into args
// --------------------------------------------------------------- //
void parse_line(char* line, struct shell_info *info, char* args[]) {
	char* saveptr;
	char* token;
	int i = 0;

	token = strtok_r(line, " ", &saveptr);  // Get first arg into token
	args[i] = malloc(strlen(token));  // Allocate memory for command
	strcpy(args[i], token);  // Copy into args
	i++;  // Argument counter

	while ((token = strtok_r(NULL, " ", &saveptr))) {  // Get rest of arguments

		if (strcmp(token, "<") == 0) {  // Identify any input file
			info->input_redirect = 1;  // Set input file flag
			token = strtok_r(NULL, " ", &saveptr);  // Get filename
			strcpy(info->input_filename, token);  // Save filename

		} else if (strcmp(token, ">") == 0) {  // Repeat for potential outfile
			info->output_redirect = 1;
			token = strtok_r(NULL, " ", &saveptr);
			strcpy(info->output_filename, token);

		} else if (strcmp(token, "&") == 0) {  // Identify background flag
			info->background = 1;

		} else if (strcmp(token, "$$") == 0) {  // Changes $$ to pid
			int pid = getpid();
			args[i] = malloc(6);
			sprintf(args[i], "%d", pid);
			i++;

		} else {
			args[i] = malloc(strlen(token));  // Bloc saves arguments for rest of line
			strcpy(args[i], token);
			i++;
		}
	}
}


// ------------------ Execute Commands Functions ----------------- //

// --------------------------------------------------------------- //
// function   : execute_cmd(..)
// parameters : char* args[]
//              struct shell_info *info
// description: Executes command stored in arguments
//              Follows arguments appropriately
// --------------------------------------------------------------- //
int execute_cmd(char* args[], struct shell_info *info) {
	int cmd;
	int status = 1;  // Return this to indicate if shell should continue

	// Check for 3 built in functions and comments/blank lines
	// If not, then execute other command via other_cmd(..)
	if (strcmp(args[0], "exit") == 0) {  // Exit
		status = my_exit();

	}	else if (strcmp(args[0], "cd") == 0) {  // Change Directory
		my_cd(args);

	} else if (strcmp(args[0], "status") == 0) {  // Status
		my_status(info->exit_status);

	}	else if (strcmp(args[0], "\n") == 0 || args[0][0] == '#') {  // Blank line or comment
		// Do nothing

	}	else {
		other_cmd(args, info);  // Execute non-built in commands
	}

	// Wait for terminated children / clean up zombies
	int corpse;
	while ((corpse = waitpid(-1, &info->exit_status, WNOHANG)) > 0) {
		printf("background pid %d is done: ", corpse);
		fflush(stdout);
		my_status(info->exit_status);  // Print how child terminated
	}

	return status;
}


// --------------------------------------------------------------- //
// function   : void other_cmd(..)
// parameters : char* args[]
//              struct shell_info *info
// description: This function executes shell commands that aren't
//              build in. Child processes are spawned and have
//              different behavior via switch statement to faciliate this
// --------------------------------------------------------------- //
void other_cmd (char* args[], struct shell_info *info) {
	pid_t spawnPid = fork();  // Fork a new process

	struct sigaction SIG_H = { 0 };  // For cusom signal handler

	switch (spawnPid) {
	case -1:
		perror("fork() \n");
		exit(1);
		break;

	case 0:  // In child process

		custom_IG();  // Children ignore SIGTSTP

		if (info->background && !stop_background) {
			printf("background pid is %d \n", getpid());  // Display background pid
			fflush(stdout);

			// Background cmd should use /dev/null for if input | output if respective redirection not specified
			if (!info->output_redirect)
				output_redirection("/dev/null");

			if (!info->input_redirect)
				input_redirection("/dev/null");

		} else {
			SIG_H.sa_handler = SIG_DFL;  // Restore ^C default functionality in child
			sigfillset(&SIG_H.sa_mask);  // Only in foreground
			SIG_H.sa_flags = SA_RESETHAND;
			sigaction(SIGINT, &SIG_H, NULL);
		}

		// ------------------ I/O Redirection ------------------ //

		if (info->input_redirect) {
			input_redirection(info->input_filename);  // Input redirection if applicable
		}

		if (info->output_redirect) {
			output_redirection(info->output_filename);  // Output redirection if applicable
		}

		// ------------------ Execute Command ------------------ //

		execvp(args[0], args);  // Replace the current program with command (aka execute command)
		perror("execvp");  // this only returns if there is an exec error
		exit(2);
		break;

	default:  // In parent process

		if (info->background && !stop_background) {  // Run in background
			spawnPid = waitpid(spawnPid, &info->exit_status, WNOHANG);

		}	else {  // Run in foreground

			spawnPid = waitpid(spawnPid, &info->exit_status, 0);  // Wait for child's termination

			if (info->exit_status != 0) {  // Print out abnormal exit if applicable
				my_status(info->exit_status);
			}

		}
		break;
	}
}


// ------------------ Signal Functions ------------------ //

// --------------------------------------------------------------- //
// function   : handle_SIG(..)
// parameters : in signo
// description: Custom handler for SIGINT / ^C
//              Children running as a foreground process terminates itself
// reference  : How can a process kill itself - https://stackoverflow.com/a/7851269/10895933
// --------------------------------------------------------------- //
void handle_SIG(int signo) {
	// stop_background is a global variable (only used for custom sig handler)
	if (stop_background == 0) {

		// Print "No more background" message
		char* msg = "Entering foreground-only mode (& is now ignored) \n";
		write(STDOUT_FILENO, msg, 50);
		fflush(stdout);

		stop_background = 1;  // set stop_background flag on

	}	else {
		// Print "Background is back on" message
		char* msg = "Exiting foreground-only mode \n";
		write(STDOUT_FILENO, msg, 30);
		fflush(stdout);

		stop_background = 0;  // Set stop_background flag off
	}
}


void custom_SIG() {
	struct sigaction SIG_IG = { 0 };  // Ignores ^C
	SIG_IG.sa_handler = SIG_IGN;	  // This is reinstated for children
	sigfillset(&SIG_IG.sa_mask);
	SIG_IG.sa_flags = SA_RESTART;
	sigaction(SIGINT, &SIG_IG, NULL);
}

void custom_SIGTSTP() {
	struct sigaction SIGINT_action = { 0 };  // Init struct to be empty
	SIGINT_action.sa_handler = handle_SIG;  // Register as the signal handler
	sigfillset(&SIGINT_action.sa_mask);  // Block all catchable signals while running
	SIGINT_action.sa_flags = SA_RESTART;  // Clear error caused by signal
	sigaction(SIGTSTP, &SIGINT_action, NULL);  // Install our signal handler
}

void custom_IG() {
	struct sigaction SIG_IG = { 0 };
	SIG_IG.sa_handler = SIG_IGN;
	sigfillset(&SIG_IG.sa_mask);
	SIG_IG.sa_flags = SA_RESTART;
	sigaction(SIGTSTP, &SIG_IG, NULL);
}

// ------------------ Shell Main Function ------------------ //

// --------------------------------------------------------------- //
// function   : small_shell()
// parameters : none
// description: Controls the flow of the small shell
//              Gets user input as string
//              Parses string into command
//              Executes command
//              Loops until user gives exit command
// --------------------------------------------------------------- //
void small_shell() {
	struct shell_info info;
	int status;
	char* args[512];

	for (int i = 0; i < 512; i++) {  // Init args to null
		args[i] = NULL;
	}

	custom_SIG();  // Set custom signal handlers
	custom_SIGTSTP();

	printf("smallsh \n");  // Displays title of program
	fflush(stdout);

	do {
		init_shell_info(&info);  // Initialize shell info to 0

		char* line = get_input();  // Gets user string input

		parse_line(line, &info, args);  // Parses input into arguments

		status = execute_cmd(args, &info);  // Executes passed in command

		free_memory(line, args);

	} while (status);
}


// ------------------ Main Function ------------------ //
int main() {
	small_shell();

	return 0;
}
