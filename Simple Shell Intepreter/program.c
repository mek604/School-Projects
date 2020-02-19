/*
	CSC 360 - Programming Assigment 1
	A simple shell interpreter
	Fall 2019

	Author: Mek Obchey

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>

#define MAX_LINE 256


struct bg_proc* root;

/*
	Print the prompt line in the following format (without a prompt cursor):
		SSI: username@hostname: <working directory>
*/
void prompt() {
	char* username = NULL;  
	char hostname[MAX_LINE];
	char cwd[MAX_LINE];
	char* prompt = (char*) malloc(MAX_LINE);

	username = getlogin();
    gethostname(hostname, MAX_LINE); 
	getcwd(cwd, sizeof(cwd));
    printf("\nSSI: %s@%s: %s ", username, hostname, cwd); 
    free(prompt);
}

/* Background process structure */
struct bg_proc {
	pid_t pid;
	char* command;
	struct bg_proc* next;
};

/*
	Split one string of arguments into a list of arguments.
	Source: CSC 360 tutorial 2, page 5.
*/
void tokenize(char* line, char* args[]) {
	int i = 0;
	char* copy = malloc(sizeof(line));
	copy = strcpy(copy, line); 
	args[0] = strtok(copy, " \n");
	while(args[i] != NULL) {
		args[i+1] = strtok(NULL, " \n");
		i++;
	}

}

int changeDirectory(char* args[]) {
	if(args[1] == NULL || strcmp(args[1], "~") == 0) {
		chdir(getenv("HOME"));
	} 
	else {
		chdir(args[1]);
	}

	return 0;
}

void print_bglist() {
	struct bg_proc* pointer = root;
	int counter = 0;
	if(pointer != NULL) {
		while(pointer != NULL) {
			printf("%d: %s\n", pointer->pid, pointer->command);
			pointer = pointer->next;
			counter += 1;
		}
	}
	printf("Total Background jobs:\t%d\n", counter);
}


/*
	Add a background process into the linked list of background processes
*/
void addLast(struct bg_proc* process) {
	struct bg_proc* pointer = root;
	struct bg_proc* prev = pointer;
	if(root == NULL)
		root = process;
	else {
		while(pointer != NULL) {
			prev = pointer;
			pointer = pointer->next;
		}
		prev->next = process;
	}
}

/*
	If a child process is terminated, reorganize the linked list of background processes.
	Given in the CSC 360 tutorial 3, page 5.
*/
int check_child() {
	pid_t ter = waitpid(0, NULL, WNOHANG);
	struct bg_proc* cur = root;
	while(ter > 0) {
		if(ter > 0) {
			if(root->pid == ter) {
				printf("%d: %s has terminated", root->pid, root->command);
				root = root->next;
			}
			else {
				cur = root;
				while(cur->next->pid == ter) {
					printf("%d: %s has terminated", cur->next->pid, cur->next->command);
					cur->next = cur->next->next;
				}
			}
		}
		ter = waitpid(0, NULL, WNOHANG);
	}
	return 0;
}

/*
	Tokenize and execute command line arguments.
*/
int execute(char* line, char* args[]) {
	pid_t pid;
	tokenize(line, args);

	if(strcmp(args[0], "cd") == 0) {
		changeDirectory(args);
	}
	else if(strcmp(args[0], "bglist") == 0) {
		print_bglist();
	}
	else if(strcmp(args[0], "bg") == 0) {
		args = args + 1; //remove bg and take next argument as a command
		pid = fork();	

		// -----------------------create a new process---------------------------------
		struct bg_proc* process = malloc(sizeof(struct bg_proc));
		process->pid = pid;
		process->command = line + 3; //exclude bg and a space
		process->next = NULL;
		// --------------------------------------------------------

		addLast(process);
		if(pid == 0)
			execvp(args[0], args);
	}
	else {
		pid = fork();
		if(pid == 0)
			execvp(args[0], args);
		wait(NULL);
	}
		
	check_child();
	return 0;
}

int main() {
	char* args[MAX_LINE];
	char* line;

	while(1) {

		prompt();
		line = readline(" > ");
    	execute(line, args);
  
  	}

  	free(args);
  	free(line);
  	
    return 0; 

}
