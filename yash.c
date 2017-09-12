#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

pid_t cpid;

int main(void) {
	while(1) {
		char str[2000];	 // MAKE 2001		
		const char s[2] = " ";
		char *token;
		char command[2000];
		char *arg;

		/* get and parse command input */
		fgets(str, 2000, stdin);
		strtok(str, "\n");
		token = strtok(str, s);
		arg = token;
		strcpy(command, "");
		int commandParsed = 0;
		int pipeCommand = 0;
		int numTokens = 0;

		while (token != NULL) {
			printf("This is the token: %s\n", token);

			/* haven't fully parsed the left command */
			if (!commandParsed && strcmp(token, "<") != 0 && strcmp(token, ">") != 0 && strcmp(token, "2>") != 0 && strcmp(token, "|") != 0) {
				strcat(command, " ");
				strcat(command, token);
				numTokens++;
			}
		 	else {
				commandParsed = 1;
				if (strcmp(token, "|") == 0) {
					pipeCommand = 1;
				}	
			}
	
			// I WORK TILL HERE!!!

			if (commandParsed) {
				char *fileToken;
				int in, out;

				// convert command into args for execvp
				char *args[numTokens + 1];
				args[0] = strtok(command, s);
				for (int i = 1; i < numTokens; i++) {
					args[i] = strtok(NULL, s);
				}
				args[numTokens] = NULL;

				// do file redirection
				if (!pipeCommand) {
					while (token != NULL) {
						printf("about to fork");
						cpid = fork();
						if (cpid == 0) {
							// child process
							if (strcmp(token, "<") == 0) {
								// replace stdin with next token
								fileToken = strtok(NULL, s);
								in = open(fileToken, O_RDONLY);
								dup2(in, STDIN_FILENO);
								printf("about to execvp");
								execvp(arg, args);
							}
							else if (strcmp(token, ">") == 0) {
								// replace stdin with next token
								fileToken = strtok(NULL, s);
								out = open(fileToken, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
								dup2(out, STDOUT_FILENO);
								printf("about to execvp");
								execvp(arg, args);
							}
							else {
								// replace stderr with the next token
					  		fileToken = strtok(NULL, s);
							}
						}
						else {
							// parent process
						}
						sleep(1);
						token = strtok(NULL, s);
					}
				}

				// do piping 
				else  {
			
				}
			}
			token = strtok(NULL, s);
		}
	}
}
