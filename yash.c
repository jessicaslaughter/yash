#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

int pid_ch1, pid_ch2, status, pid;
int pipefd[2];


void spaceToNull(char array[]);

void createArgv(char array[], char *argarray[], int num, int start);

void doFileRedirection (char *inFile, char *outFile, char *errFile, int in, int out, int err, char *argv[]);

void doPiping(char str[], int numArgs, int numSecArgs);

void defaultExec (char array[], int num);


static void sig_int(int signo) {
		printf("Sending signals to group: %d\n", pid_ch1);
		kill(-pid_ch1, SIGINT);
}

static void sig_tstp(int signo) {
		printf("Sending SIGTSTP to group: %d\n", pid_ch1);
		kill(-pid_ch1, SIGTSTP);
}

static void sig_cont(int signo) {
		printf("Sending CONT to %d\n", pid);
		sleep(4);
		kill(pid, SIGCONT);
}

void shellInit() {
		signal(SIGINT, SIG_IGN);
		signal(SIGTSTP, SIG_IGN);
}

int main(void) {		
		shellInit();
		while(1) {	
				const int INPUT_SIZE = 2001;
				char str[INPUT_SIZE];		
				const char delimeter[2] = " ";
				char *currentTok;
				char *prevTok;
				char *inFile = NULL;
				char *outFile = NULL;
				char *errFile = NULL;
				char *ch1, *ch2;
				int numArgs = 0;
				int numSecArgs = 0;
				/* flags */
				int cmdParsed = 0;
				int fileRedirect = 0;
				int piping = 0;
				
				/* get and parse command input */
				printf("# ");
				sleep(.2);
				if(fgets(str, INPUT_SIZE, stdin) == NULL) { // check if ctrl-d was pressed 
						printf("\n");
						exit(EXIT_SUCCESS);
				}
				strtok(str, "\n");
				currentTok = strtok(str, delimeter);
				numArgs++;
				ch1 = currentTok;

				while (currentTok != NULL) {
						prevTok = currentTok;
						currentTok = strtok(NULL, delimeter);
						if (strcmp(prevTok, "<") == 0) {
								// need to replace stdin
								inFile = currentTok;
								cmdParsed = 1;
								fileRedirect = 1;
						}
						else if (strcmp(prevTok, ">") == 0) {
								// need to replace stdout
								outFile = currentTok;
								cmdParsed = 1;
								fileRedirect = 1;
						}
						else if (strcmp(prevTok, "2>") == 0) {
								// need to replace stdout
								errFile = currentTok;
								cmdParsed = 1;
								fileRedirect = 1;
						}
						else if (strcmp(prevTok, "|") == 0) {
								ch2 = currentTok;
								piping = 1;
								cmdParsed = 1;
						}
						else if (!cmdParsed) {
								// command hasn't been parsed
								numArgs++;
						}

						if (piping) {
								numSecArgs++;
						}
				}

				if (fileRedirect) {
						char *argv[numArgs];
						spaceToNull(str);
						createArgv(str, argv, numArgs, 0);
						
						/* open all files given in input */
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

						doFileRedirection(inFile, outFile, errFile, in, out, err, argv);
				}
				
				
				else if (piping) {
						doPiping(str, numArgs, numSecArgs);		
				}
				else { // default execvp
						defaultExec(str, numArgs);		
				}
		}
}


/* change spaces to null characters for easier string access */
void spaceToNull(char array[]) {

		int i = 0;
		int count = 0;
		
		while (array[i] != '\0') {
				if (array[i] == ' ') {
						array[i] = '\0';
				}
				i++;
		}
}

/* create argv array for use in execvp */
void createArgv(char array[], char *argarray[], int num, int start) {
		
		char *ptr = array;
		for (int i = 0; i < start; i++) {
				ptr = ptr + strlen(ptr) + 1;
		}
		for (int j = 0; j < num - 1; j++) {
				argarray[j] = ptr;
				ptr = ptr + strlen(ptr) + 1;
		}
		argarray[num - 1] = NULL;
}

/* create child and perform file redirection */
void doFileRedirection (char *inFile, char *outFile, char *errFile, int in, int out, int err, char *argv[]) {
		
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

/* create two children and perform piping */
void doPiping(char str[], int numArgs, int numSecArgs) {

		if (pipe(pipefd) == -1) {
				perror("pipe");
				exit(EXIT_FAILURE);
		}

		pid_ch1 = fork();
		if (pid_ch1 > 0) {
				printf("child 1 pid: %d\n", pid_ch1);
				// parent
				pid_ch2 = fork();
				if (pid_ch2 > 0) {
						printf("child 2 pid: %d\n", pid_ch2);
						if (signal(SIGINT, sig_int) == SIG_ERR) {
								printf("signal(SIGINT) error");
						}
						if (signal(SIGTSTP, sig_tstp) == SIG_ERR) {
								printf("signal(SIGTSTP) error");
						}
						close(pipefd[0]); // close pipe in the parent
						close(pipefd[1]);
						int count = 0;
						while (count < 2) {
								pid = waitpid(-1, &status, WUNTRACED | WCONTINUED);
								if (pid == -1) {
										perror("waitpid");
										exit(EXIT_FAILURE);
								}
								if (WIFEXITED(status)) {
										printf("child %d exited, status = %d\n", pid, WEXITSTATUS(status));
										count++;
								} else if (WIFSIGNALED(status)) {
										printf("child %d killed by signal %d\n", pid, WTERMSIG(status));
										count++;
								} else if (WIFSTOPPED(status)) {
										printf("%d stopped by signal %d\n", pid, WSTOPSIG(status));
										signal(SIGCONT, sig_cont);
								} else if (WIFCONTINUED(status)) {
										printf("Continuing %d\n", pid);
								}
						}
						
				} else {
						// child 2
						sleep(1);
						setpgid(0, pid_ch1); // ch2 joins group with ch1
						close(pipefd[1]); // close write end
						dup2(pipefd[0], STDIN_FILENO); // replace stdin with output from pipe
						char *argv2[numSecArgs];
						createArgv(str, argv2, numSecArgs, numArgs);
						execvp(argv2[0], argv2);
				}
		} else {
				// child 1
				setsid();
				close(pipefd[0]); // close read end
				dup2(pipefd[1], STDOUT_FILENO); // replace stdout with input from pipe
				char *argv1[numArgs];
				createArgv(str, argv1, numArgs, 0);
				execvp(argv1[0], argv1);
		}
}

/* handle a command that doesn't fit file redirection / pipes / jobs */
void defaultExec (char array[], int num) {	
		int pid = fork();
		if (pid == 0) {
				char *argvdefault[num];
				createArgv(array, argvdefault, num, 0);
				execvp(argvdefault[0], argvdefault);
		}
		else {
				// parent
				wait(NULL);
		}
}
