#include "Console.h"
/*
* 
    //==============================================================================
    // File: Console Example/Console Example.cpp
    // Description: This file contains the implementation of a simple shell program
    //              that supports command execution, input/output redirection, 
    //              background processes, and conditional command execution.
    //==============================================================================

    #include "Console.h"

    /**
     * @brief Main function of the shell program.
     *
     * This function initializes the signal handler, reads user input, tokenizes it,
     * and executes the corresponding commands. It supports built-in commands like
     * 'exit', 'status', and 'cd', as well as external command execution with various
     * modes such as background execution and I/O redirection.
     *
     * @return int Exit status of the program.
     */

int main() {
	char input[MAX_INPUT_SIZE];
	char* tokens[MAX_TOKENS];
	int tokenCount = 0;
	int mode;

	// Setup signal handler for SIGCHLD
	struct sigaction sa;
	sa.sa_handler = signalHandler;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGCHLD, &sa, NULL);

	while (true) {
		// Prompt user for input
		printf("bash> ");
		if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
			perror("Error reading input");
			continue;
		}

		// Trim newline and tokenize the input
		trimNewline(input);
		tokenizeInput(input, tokens, &tokenCount);

		if (tokenCount == 0) {
			continue; // Ignore empty input
		}

		// Handle built-in commands
		if (strcmp(tokens[0], "exit") == 0) {
			break; // Exit the shell
		}
		else if (strcmp(tokens[0], "status") == 0) {
			if (tokenCount == 1) {
				printLastProcessStatus();
			}
			else if (tokenCount == 2) {
				int jobNumber = atoi(tokens[1]);
				if (jobNumber >= 0 && jobNumber < jobCount) {
					Process job = jobList[jobNumber];
					if (job.terminationType == 1) {
						printf("Process %d exited with status %d\n", job.jobNumber, job.exitStatus);
					}
					else if (job.terminationType == 2) {
						printf("Process %d terminated by signal %d\n", job.jobNumber, job.exitStatus);
					}
					else {
						printf("Process %d is still running\n", job.jobNumber);
					}
				}
				else {
					printf("Invalid job number\n");
				}
			}
			else {
				fprintf(stderr, "Usage: status [jobNumber]\n");
			}
		}
		else if (strcmp(tokens[0], "cd") == 0) {
			changeDirectory(tokens);
		}
		else {
			// Parse command and execute
			mode = parseCommand(tokens, tokenCount);
			executeCommand(tokens, mode, tokenCount); // Updated function to include tokenCount
		}
	}

	return 0;
}

/**
 * @brief Prints the status of the last terminated process.
 *
 * This function prints the status of the last terminated process, including the exit status
 * or the signal that caused the termination.
 */
void printLastProcessStatus() {
	if (lastTerminationType == 1) {
		printf("Last process exited with status: %d\n", lastExitStatus);
	}
	else if (lastTerminationType == 2) {
		printf("Last process terminated by signal: %d\n", lastExitStatus);
	}
	else {
		printf("No process status available\n");
	}
}
/**
 * @brief Trims the newline character from a string.
 *
 * This function trims the newline character from the end of a string.
 *
 * @param str Input string.
 * @return void
 */
void changeDirectory(char* args[]) {
	if (args[1] == NULL) {
		fprintf(stderr, "cd: missing argument\n");
	}
	else if (chdir(args[1]) != 0) {
		perror("cd");
	}
}
/**
* @brief Parses the command and determines the execution mode.
 *
 * This function parses the command tokens and determines the execution mode based on the
 * presence of special characters such as "&", ">", ">>", and "<".
 *
 * Supported modes:
 * - 0: Normal execution
 * - 1: Background process
 * - 2: Redirect output to file (overwrite)
 * - 3: Redirect output to file (append)
 * - 4: Redirect input from file
 * - 5: Pipe commands
 * - 6: Execute next command on success
*/
int parseCommand(char* tokens[], int tokenCount) {
	for (int i = 0; i < tokenCount; i++) {
		if (strcmp(tokens[i], ">") == 0) return 2;
		if (strcmp(tokens[i], ">>") == 0) return 3;
		if (strcmp(tokens[i], "<") == 0) return 4;
		if (strcmp(tokens[i], "&") == 0) return 1;
	}
	return 0;
}

/**
 * @brief Executes a command based on the specified mode.
 *
 * This function directs the behavior of the command execution based on the provided mode.
 * Supported modes:
 * - 0: Normal execution
 * - 1: Background process
 * - 2: Redirect output to file (overwrite)
 * - 3: Redirect output to file (append)
 * - 4: Redirect input from file
 * - 5: Pipe commands
 * - 6: Execute next command on success
 * - 7: Execute next command on failure
 * - 8: Change directory
 *
 * @param tokens Array of command arguments.
 * @param mode Execution mode.
 */
void executeCommand(char* tokens[], int mode, int index) {
	switch (mode) {
	case 0: // Normal execution
		forkAndExecute(tokens, mode);
		break;

	case 1: // Background process
		tokens[index] = strtok(tokens[index], "&");
		forkAndExecute(tokens, mode);
		break;

	case 2: 
		forkAndExecute(tokens, mode);
		break;
	case 3:
		forkAndExecute(tokens, mode);
		break;
	case 4: // Redirect input from file
		forkAndExecute(tokens, mode);
		break;

	case 5: // Execute next command on success
		handlePiping(tokens, index);
		break;

	case 6: // Execute next command on failure
		executeConditional(tokens, 1, index);
		break;

	case 7: // Pipe commands
		executeConditional(tokens, 2, index);
		break;

	case 8: // Change directory
		changeDirectory(tokens);
		break;

	default:
		perror("Unknown mode in executeCommand");
	}
}
/**
* @brief Forks a new process and executes the command.
 *
 * This function forks a new process and executes the command in the child process.
 * The parent process waits for the child process to complete, unless the command is
 * running in the background.
 *
 * @param tokens Array of command arguments.
 * @param mode Execution mode.
 */

void executeWithRedirection(char* tokens[], int mode) {
	int fd;
	char* file = NULL;

	for (int i = 0; tokens[i] != NULL; i++) {
		if (strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], ">>") == 0 || strcmp(tokens[i], "<") == 0) {
			file = tokens[i + 1];
			tokens[i] = NULL; // Terminate command before redirection operator
			break;
		}
	}

	if (!file) {
		fprintf(stderr, "Missing file for redirection\n");
		exit(EXIT_FAILURE);
	}

	if (mode == 2) {
		fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	}
	else if (mode == 3) {
		fd = open(file, O_WRONLY | O_CREAT | O_APPEND, 0644);
	}
	else {
		fd = open(file, O_RDONLY);
	}

	if (fd < 0) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	if (dup2(fd, (mode == 4 ? 0 : 1)) < 0) {
		perror("dup2");
		exit(EXIT_FAILURE);
	}

	close(fd);
	execvp(tokens[0], tokens);
	perror("execvp");
	exit(EXIT_FAILURE);
}
/**
 * @brief Signal handler for handling terminated child processes.
 *
 * This function is called when a child process terminates. It updates the last process status
 * with the exit status or termination signal of the child process.
 *
 * @param signal Signal number.
* 
*/
void signalHandler(int signal) {
	int status;
	pid_t pid;

	while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
		lastProcessID = pid;
		if (WIFEXITED(status)) {
			lastExitStatus = WEXITSTATUS(status);
			lastTerminationType = 1;
		}
		else if (WIFSIGNALED(status)) {
			lastExitStatus = WTERMSIG(status);
			lastTerminationType = 2;
		}
	}
}
/**
 * @brief Waits for a process to complete and updates the last process status.
 *
 * If the process is running in the background, a message is printed indicating that
 * the process has started in the background. Otherwise, the function waits for the
 * process to complete and updates the last process status with the exit status or
 * termination signal.
 *
 * @param pid Process ID.
 * @param background Flag indicating whether the process is running in the background.
 */ /
void waitForProcess(pid_t pid, bool background) {
	if (background) {
		printf("Process %d started in the background\n", pid);
	}
	else {
		int status;
		if (waitpid(pid, &status, 0) == -1) {
			perror("waitpid");
		}
		else if (WIFEXITED(status)) {
			lastExitStatus = WEXITSTATUS(status);
			lastTerminationType = 1;
		}
		else if (WIFSIGNALED(status)) {
			lastExitStatus = WTERMSIG(status);
			lastTerminationType = 2;
		}
	}
}

char* trimNewline(char* str) {
	size_t len = strlen(str);
	if (len > 0 && str[len - 1] == '\n') {
		str[len - 1] = '\0';
	}
	return str;
}
/**
* @brief Tokenizes the input string into an array of tokens.
 *
 * This function tokenizes the input string based on the space character and stores
 * the tokens in the provided array. The token count is updated with the number of tokens
 * found in the input string.
 *
 * @param input Input string.
 * @param tokens Array to store the tokens.
 * @param tokenCount Pointer to the token count.
 */
void tokenizeInput(char* input, char* tokens[], int* tokenCount) {
	*tokenCount = 0;
	char* token = strtok(input, " ");
	while (token != NULL) {
		tokens[(*tokenCount)++] = token;
		token = strtok(NULL, " ");
	}
	tokens[*tokenCount] = NULL;
}
/**
 * @brief Executes a conditional command based on success or failure of the first command.
 *
 * Splits the input into two commands separated by `&&` or `||` and executes them
 * conditionally based on the exit status of the first command.
 *
 * @param input Array of command arguments.
 * @param mode Execution mode:
 *             - 1 (`&&`) Execute the second command only if the first succeeds.
 *             - 2 (`||`) Execute the second command only if the first fails.
 * @param numTokens Number of tokens in the input array.
 * @return 0 on success, or a fork error.
 */
int executeConditional(char* input[], int mode, int numTokens) {
	char separator[10];
	char* firstCommand[4096];
	char* secondCommand[4096];
	bool foundSeparator = false;
	int i = 0;
	int w = 0;
	int z = 0;
	pid_t childPid;

	// Determine the separator based on the mode
	if (mode == 1) {
		strcpy(separator, "&&");
	}
	else if (mode == 2) {
		strcpy(separator, "||");
	}
	else {
		fprintf(stderr, "Invalid mode for executeConditional\n");
		return -1;
	}

	// Locate the separator and split commands
	for (i = 0; foundSeparator == false && i < numTokens; i++) {
		if (strcmp(separator, input[i]) == 0) {
			foundSeparator = true;
		}
	}
	if (!foundSeparator) {
		fprintf(stderr, "Separator '%s' not found in input\n", separator);
		return -1;
	}

	i--; // Backtrack to the position of the separator
	input[i] = NULL; // Terminate the first command

	// Populate the first command array
	firstCommand[0] = input[0];
	if (i > 1) {
		for (w = 1; w < i; w++) {
			firstCommand[w] = input[w];
		}
	}
	firstCommand[w] = NULL; // Terminate the array

	// Populate the second command array
	for (z = 0, w = i + 1; w < numTokens; w++, z++) {
		secondCommand[z] = input[w];
	}
	secondCommand[z] = NULL; // Terminate the array

	// Fork to execute the first command
	switch (childPid = fork()) {
	case -1:
		perror("Fork error");
		return -1;

	case 0: // Child process
		execvp(firstCommand[0], firstCommand);
		perror("execvp"); // Only reached if execvp fails
		exit(EXIT_FAILURE);

	default: // Parent process
		checkProcess(childPid, secondCommand, mode);
		break;
	}

	return 0;
}


/**
 * @brief Forks a child process to execute a command with support for various modes.
 *
 * Handles normal execution, background execution, and I/O redirection:
 * - Mode 0: Normal execution.
 * - Mode 1: Background execution.
 * - Mode 2: Redirect output to a file (overwrite).
 * - Mode 3: Redirect output to a file (append).
 * - Mode 4: Redirect input from a file.
 *
 * @param tokens Array of command arguments.
 * @param mode Execution mode (0 to 4).
 * @return int 0 on success, -1 on error.
 */
int forkAndExecute(char* tokens[], int mode) {
	pid_t childPid;

	// Validate the input
	if (tokens[0] == NULL || tokens[0][0] == '\0') {
		fprintf(stderr, "Error: No command provided.\n");
		return -1;
	}

	// Fork the process
	childPid = fork();
	if (childPid < 0) {
		perror("Error: Fork failed");
		return -1;
	}

	if (childPid == 0) {
		// Child process
		switch (mode) {
		case 0: // Normal execution
			execvp(tokens[0], tokens);
			perror("Error: Command execution failed");
			exit(EXIT_FAILURE);
		case 1: // Background execution (same as normal execution)
			execvp(tokens[0], tokens);
			perror("Error: Command execution failed");
			exit(EXIT_FAILURE);
		case 2: // Redirect output to file (overwrite)
		case 3: // Redirect output to file (append)
		case 4: // Redirect input from file
			handleRedirection(tokens, mode);
			break;
		default:
			fprintf(stderr, "Error: Invalid mode %d\n", mode);
			exit(EXIT_FAILURE);
		}
	}
	else {
		// Parent process
		if (mode == 0 || mode == 2 || mode == 3 || mode == 4) {
			monitorProcess(childPid, /* background */ false);
		}
		else if (mode == 1) {
			printf("Background process started with PID: %d\n", childPid);
			monitorProcess(childPid, /* background */ true);
		}
	}

	return 0;
}
/**
 * @brief Handles piping between commands.
 *
 * Forks child processes to set up pipes for inter-process communication.
 *
 * @param tokens Array of command arguments.
 * @param index Index of the pipe symbol in the tokens array.
 */
void handlePiping(char* tokens[], int index) {
	int pipeFd[2];
	if (pipe(pipeFd) == -1) {
		perror("pipe");
		return;
	}

	pid_t pid1 = fork();
	if (pid1 < 0) {
		perror("fork");
		return;
	}

	if (pid1 == 0) {
		// First command (writes to pipe)
		close(pipeFd[0]);
		dup2(pipeFd[1], STDOUT_FILENO);
		close(pipeFd[1]);

		char* firstCommand[index + 1];
		for (int i = 0; i < index; i++) {
			firstCommand[i] = tokens[i];
		}
		firstCommand[index] = NULL;

		execvp(firstCommand[0], firstCommand);
		perror("execvp");
		exit(EXIT_FAILURE);
	}

	pid_t pid2 = fork();
	if (pid2 < 0) {
		perror("fork");
		return;
	}

	if (pid2 == 0) {
		// Second command (reads from pipe)
		close(pipeFd[1]);
		dup2(pipeFd[0], STDIN_FILENO);
		close(pipeFd[0]);

		char* secondCommand[4096];
		int j = 0;
		for (int i = index + 1; tokens[i] != NULL; i++) {
			secondCommand[j++] = tokens[i];
		}
		secondCommand[j] = NULL;

		execvp(secondCommand[0], secondCommand);
		perror("execvp");
		exit(EXIT_FAILURE);
	}

	// Parent process
	close(pipeFd[0]);
	close(pipeFd[1]);
	waitpid(pid1, NULL, 0);
	waitpid(pid2, NULL, 0);
}
/**
 * @brief Handles input or output redirection for a command.
 *
 * Supports modes for redirecting input from or output to files.
 *
 * @param tokens Array of command arguments.
 * @param mode Redirection mode (output overwrite, output append, input).
 */
void handleRedirection(char* tokens[], int mode) {
	int fd;
	char* fileName = tokens[2];

	if (mode == 2) {
		fd = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0644); // Overwrite
	}
	else if (mode == 3) {
		fd = open(fileName, O_WRONLY | O_CREAT | O_APPEND, 0644); // Append
	}
	else if (mode == 4) {
		fd = open(fileName, O_RDONLY); // Input redirection
	}
	else {
		perror("Unsupported redirection mode");
		exit(EXIT_FAILURE);
	}

	if (fd < 0) {
		perror("File open error");
		exit(EXIT_FAILURE);
	}

	// Redirect input/output
	if (mode == 4) {
		dup2(fd, STDIN_FILENO);
	}
	else {
		dup2(fd, STDOUT_FILENO);
	}
	close(fd);

	// Execute command
	execvp(tokens[0], tokens);
	perror("execvp");
	exit(EXIT_FAILURE);
}
/**
 * @brief Monitors the child process and handles background execution if needed.
 *
 * This function waits for the child process to complete unless it is a background process.
 *
 * @param pid Process ID of the child process.
 * @param isBackground Whether the process should run in the background.
 */
void monitorProcess(pid_t pid, bool isBackground) {
	if (isBackground) {
		printf("Background process started with PID: %d\n", pid);
	}
	else {
		int status;
		if (waitpid(pid, &status, 0) == -1) {
			perror("waitpid");
		}
		else if (WIFEXITED(status)) {
			printf("Process %d exited with status %d\n", pid, WEXITSTATUS(status));
		}
		else if (WIFSIGNALED(status)) {
			printf("Process %d terminated by signal %d\n", pid, WTERMSIG(status));
		}
	}
}
/**
 * @brief Checks the exit status of a child process and executes the second command conditionally.
 *
 * Executes the second command based on the success or failure of the first command,
 * depending on the specified mode.
 *
 * @param pid Process ID of the child process.
 * @param secondCommand Array of arguments for the second command.
 * @param mode Execution mode:
 *             - 1 (`&&`) Execute if the first command succeeds.
 *             - 2 (`||`) Execute if the first command fails.
 */
void checkProcess(pid_t pid, char* secondCommand[], int mode) {
	int status;

	waitpid(pid, &status, 0); // Wait for the first command to finish

	if (WIFEXITED(status)) {
		int exitStatus = WEXITSTATUS(status);
		if ((mode == 1 && exitStatus == 0) || (mode == 2 && exitStatus != 0)) {
			// Execute the second command
			pid_t secondPid = fork();
			if (secondPid < 0) {
				perror("Fork error for second command");
				return;
			}

			if (secondPid == 0) {
				execvp(secondCommand[0], secondCommand);
				perror("execvp"); // Only reached if execvp fails
				exit(EXIT_FAILURE);
			}

			// Wait for the second command to finish
			waitpid(secondPid, NULL, 0);
		}
	}
}
/**
 * @brief Executes a command using execvp.
 *
 * This function is used for standard execution of commands without any redirection or additional handling.
 *
 * @param tokens Array of command arguments.
 */
void runCommand(char* tokens[]) {
	execvp(tokens[0], tokens);
	perror("execvp");
	exit(EXIT_FAILURE); // Exit if execvp fails
}
/**
 * @brief Handles input/output redirection for commands.
 *
 * Redirects output to a file (overwrite or append) or input from a file
 * based on the provided mode.
 *
 * @param tokens Array of command arguments.
 * @param mode Redirection mode:
 *             - 2: Redirect output to file (overwrite)
 *             - 3: Redirect output to file (append)
 *             - 4: Redirect input from file
 */
void handleRedirection(char* tokens[], int mode) {
	const char* separator = (mode == 2) ? ">" : (mode == 3) ? ">>" : "<";
	char* command[4096];
	char* file;
	int fd, redirectFd;
	int i = 0, j = 0;
	bool found = false;

	// Parse tokens to separate command and file
	while (tokens[i] != NULL) {
		if (strcmp(tokens[i], separator) == 0) {
			found = true;
			break;
		}
		command[j++] = tokens[i++];
	}

	if (!found || tokens[i + 1] == NULL) {
		fprintf(stderr, "Error: Invalid redirection syntax.\n");
		exit(EXIT_FAILURE);
	}

	command[j] = NULL; // Null-terminate command
	file = tokens[i + 1];

	// Open file based on mode
	if (mode == 2) { // Overwrite
		fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0755);
	}
	else if (mode == 3) { // Append
		fd = open(file, O_WRONLY | O_CREAT | O_APPEND, 0755);
	}
	else { // Input redirection
		fd = open(file, O_RDONLY);
	}

	if (fd < 0) {
		perror("Error opening file");
		exit(EXIT_FAILURE);
	}

	// Set redirection
	redirectFd = (mode == 4) ? STDIN_FILENO : STDOUT_FILENO;
	if (dup2(fd, redirectFd) < 0) {
		perror("Error setting redirection");
		close(fd);
		exit(EXIT_FAILURE);
	}
	close(fd);

	// Execute the command
	execvp(command[0], command);
	perror("Error executing command");
	exit(EXIT_FAILURE);
}







