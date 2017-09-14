#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

int cpid;

int main(void) {
		while(1) {
				const int INPUT_SIZE = 2001;
				char str[INPUT_SIZE];		
				const char delimeter[2] = " ";
				char *currentTok;
				char *prevTok;
				char *inFile = NULL;
				char *outFile = NULL;
				char *errFile = NULL;
				char *arg;

				int cmdParsed = 0;
				int numArgs = 0;

				/* get and parse command input */
				fgets(str, INPUT_SIZE, stdin);
				strtok(str, "\n");
				currentTok = strtok(str, delimeter);
				arg = currentTok;
				numArgs = 1;

				while (currentTok != NULL) {
						printf("This is the token: %s\n", currentTok);
						prevTok = currentTok;
						currentTok = strtok(NULL, delimeter);
						if (strcmp(prevTok, "<") == 0) {
								// need to replace stdin
								inFile = currentTok;
								cmdParsed = 1;
						}
						else if (strcmp(prevTok, ">") == 0) {
								// need to replace stdout
								outFile = currentTok;
								cmdParsed = 1;
						}
						else if (strcmp(prevTok, "2>") == 0) {
								// need to replace stdout
								errFile = currentTok;
								cmdParsed = 1;
						}
						else if (!cmdParsed) {
								numArgs++;
						}
				}

				printf("num args: %d\n", numArgs);
				int i = 0;
				int count = 0;
				
				while (str[i] != '\0') {
						if (str[i] == ' ') {
								str[i] = '\0';
						}
						i++;
				}
				printf("inFile: %s\n", inFile);
				printf("outFile: %s\n", outFile);
				printf("errFile: %s\n", errFile);
				
				char *argv[numArgs];
				char *ptr = str;
				for (int i = 0; i < numArgs - 1; i++) {
						argv[i] = ptr;
						ptr = ptr + strlen(ptr) + 1;
				}
				
				int in, out, err;
				if (inFile != NULL) {
						in = open(inFile, O_RDONLY);
				}
			  if (outFile != NULL) {
						out = open(outFile, O_WRONLY | O_CREAT, S_IWUSR);
				}
				if (errFile != NULL) {
						err = open(errFile, O_WRONLY | O_CREAT, S_IWUSR);
				}
				
				int cpid;
				cpid = fork();
				if (cpid == 0) {
						// child
						if (inFile != NULL) {
								dup2(in, STDIN_FILENO);
								close(*inFile);
						}		
						if (outFile != NULL) {
								dup2(out, STDOUT_FILENO);
								close(*outFile);
						}
						if (errFile != NULL) {
								dup2(err, STDERR_FILENO);
								close(*errFile);
						}
						execvp(argv[0], argv);
				}
				else {
						// parent
				}
		}
}
