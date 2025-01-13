// Standard Libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

// Constants
#define MAX_JOBS 65000
#define MAX_INPUT_SIZE 4096
#define MAX_TOKENS 1024

// Process structure
typedef struct {
	int jobNumber;
	int memoryUsage;
	int terminationType; // 0 = running, 1 = normal exit, 2 = signal exit
	int exitStatus;
	pid_t processID;
} Process;

Process jobList[MAX_JOBS];
int jobCount = 0;

// Global variables for tracking the last terminated process
pid_t lastProcessID = 0;
int lastTerminationType = 0;
int lastExitStatus = 0;

// Function prototypes
void changeDirectory(char* args[]);
void checkProcess(pid_t pid, char* secondCommand[], int mode);
void executeCommand(char* tokens[], int mode, int index);
int executeConditional(char* input[], int mode, int numTokens);
void executeWithRedirection(char* tokens[], int mode);
int forkAndExecute(char* tokens[], int mode);
void handlePiping(char* tokens[], int index);
void handleRedirection(char* tokens[], int mode);
void monitorProcess(pid_t pid, bool isBackground);
void printLastProcessStatus(void);
void runCommand(char* tokens[]);
void signalHandler(int signal);
void tokenizeInput(char* input, char* tokens[], int* tokenCount);
char* trimNewline(char* str);
void waitForProcess(pid_t pid, bool background);
int parseCommand(char* tokens[], int tokenCount);
