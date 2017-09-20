#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#define STACK_SIZE 100

pid_t pid_ch1, pid_ch2, pid, pid_shell, pid_current, pid_default;
int pipefd[2];
int stack[STACK_SIZE];
int top = -1;
int status;
siginfo_t *infop;

void spaceToNull(char array[]);

void createArgv(char array[], char *argarray[], int num, int start);

void doFileRedirection (char *inFile, char *outFile, char *errFile, int in, int out, int err, char *argv[]);

void doPiping(char str[], int numArgs, int numSecArgs);

void doBackground(char str[], int num);

void defaultExec (char array[], int num, int bg);

void foregroundJob();

void backgroundJob();

void waitForJob(int pid);

int isEmpty();

int isFull();

int peek();

int pop();

int push(int data);

static void sig_int(int signo) {
		kill(-pid_ch1, SIGINT);
}

static void sig_tstp(int signo) {
		kill(-pid_ch1, SIGTSTP);
}

static void sig_tstp2(int signo) {
		kill(pid_default, SIGTSTP);
}

static void sig_cont(int signo) {
		sleep(4);
		kill(pid, SIGCONT);
}

void shellInit() {
		signal(SIGINT, SIG_IGN);
		signal(SIGTSTP, SIG_IGN);
		signal(SIGTTOU, SIG_IGN);
		pid_shell = getpid();
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
				int background = 0;
				int fgJob = 0;
				int bgJob = 0;

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
						else if (strcmp(prevTok, "&") == 0) {
								background = 1;
								cmdParsed = 1;
						}
						else if (strcmp(prevTok, "fg") == 0) {
								fgJob = 1;
						}
						else if (strcmp(prevTok, "bg") == 0) {
								bgJob = 1;
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

				else if (background) {
						doBackground(str, numArgs);
				}
				
				else if (fgJob) {
						foregroundJob();
				}

				else if (bgJob) {
						backgroundJob();
				}

				else { // default execvp
						defaultExec(str, numArgs, 0);		
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
				// parent
				pid_current = pid_ch1;
				pid_ch2 = fork();
				if (pid_ch2 > 0) {
						if (signal(SIGINT, sig_int) == SIG_ERR) {
								printf("signal(SIGINT) error");
						}
						if (signal(SIGTSTP, sig_tstp) == SIG_ERR) {
								printf("signal(SIGTSTP) error");
						}
						if (signal(SIGCONT, sig_cont) == SIG_ERR) {
								printf("signal(SIGCONT) error");
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
										count++;
								} else if (WIFSIGNALED(status)) {
										count++;
								} else if (WIFSTOPPED(status)) {
										push(pid_ch1);
										return;
								} else if (WIFCONTINUED(status)) {
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

/* place input command in the background */
void doBackground(char str[], int num) {
		int i = 0;
		while (str[i] != '\0') {
				if (str[i] == '&') {
						str[i] = '\0';
				}
				i++;
		}
		// run this command in the background
		defaultExec(str, num, 1);

}

/* handle a command that doesn't fit file redirection / pipes / jobs */
void defaultExec (char array[], int num, int bg) {	
		pid_default = fork();
		if (pid_default > 0) {
				if (bg) {
						kill(pid_default, SIGCONT);
				}
				else {
						if (signal(SIGINT, sig_int) == SIG_ERR) {
								printf("signal(SIGINT) error");
						}
						if (signal(SIGTSTP, sig_tstp2) == SIG_ERR) {
								printf("signal(SIGTSTP) error");
						}
						if (signal(SIGCONT, sig_cont) == SIG_ERR) {
								printf("signal(SIGCONT) error");
						}
						int count = 0;
						while (count < 1) {
								pid = waitpid(-1, &status, WUNTRACED | WCONTINUED);
								if (pid == -1) {
										perror("waitpid");
										exit(EXIT_FAILURE);
								}
								if (WIFEXITED(status)) {
										count++;
								} else if (WIFSIGNALED(status)) {
										count++;
								} else if (WIFSTOPPED(status)) {
										push(pid_default);
										return;
								} else if (WIFCONTINUED(status)) {
								}
						}
				}
		}
		else {
				// child
				char *argvdefault[num];
				createArgv(array, argvdefault, num, 0);
				execvp(argvdefault[0], argvdefault);
		}
}

void foregroundJob() {
		FILE *fp;
		int fd, job;

		fp = fopen("/dev/tty", "r");
		fd = fileno(fp);
		job = pop();
		tcsetpgrp(fd, job);
		if (kill(-job, SIGCONT) < 0) {
				perror("kill (SIGCONT)");
		}	
		int count = 0;
		while (count < 2) {
				pid = waitpid(-1, &status, WUNTRACED | WCONTINUED);
				if (pid == -1) {
						perror("waitpid");
						exit(EXIT_FAILURE);
				}
				if (WIFEXITED(status)) {
						count++;
				} else if (WIFSIGNALED(status)) {
						count++;
				} else if (WIFSTOPPED(status)) {
						push(pid_ch1);
						return;
				} else if (WIFCONTINUED(status)) {
				}
		}
		tcsetpgrp(fd, pid_shell);
}

void backgroundJob() {
		int pid_bg, fd;
		FILE *fp;
		fp = fopen("/dev/tty", "r");
		fd = fileno(fp);
		pid_bg = peek();
		tcsetpgrp(fd, pid_shell);
		if (kill(pid_bg, SIGCONT) < 0) {
				perror("kill (SIGCONT)");
		}	
		tcsetpgrp(fd, pid_shell);
		return;	
}

int isEmpty() {
		if (top == -1) {
				return 1;
		}
		else {
				return 0;
		}
}

int isFull() {
		if (top == STACK_SIZE) {
				return 1;
		}
		else {
				return 0;
		}
}

int peek() {
		return stack[top];
}

int pop() {
		int data;
		if (!isEmpty()) {
				data = stack[top];
				top = top - 1;
				return data;
		}
		else {
				// stack is empty
		}
}

int push(int data) {
		if (!isFull()) {
				top = top + 1;
				stack[top] = data;
		}
		else {
				// stack is full;
		}
}
