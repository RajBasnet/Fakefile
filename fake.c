#include <error.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "recipe.h"

//Global variables defined for program
#define stack_length 1000
#define EMPTY 0
#define num_L 1000

//Global declaration of variables and functions to be utilized in the main program
void createTree(const char *, int);
static void pipeline(char ***);
static int redirect(char **);
struct recipe **recipes = NULL;
int num_recipes;
int d[num_L];
int c[num_L];
int ks1[num_L];
struct stack s1;
struct stack s2;
struct queue myTree;

//Structure of commands to hold the command line arguments to be fed into pipes or for execution of the commands
struct pcommand{
	
	char *command;
	char *comm[10];
	
};

//Stack structure created for the program
struct stack{
	char *data[stack_length];
	int top;
};

//Function to add new data to the stack
bool push(struct stack *tree, char *value){

	if(tree->top >= stack_length-1){
		return false;
	}
	
	//For adding data to the top of stack until the length limit of stack is reached
	tree->top++;
	tree->data[tree->top] = value;
	return true;
}

//Function to remove data at the top from stack
char *pop(struct stack *tree){
	
	if(tree->top == EMPTY){
		return NULL;
	}
	
	//When stack is not empty, remove the data from stack based on Last In First Out format
	char *temp = tree->data[tree->top];
	tree->top--;
	return temp;
}

//Function that returns the size of stack
int size(struct stack *tree){
	
	return tree->top;
}

//Queue Data structure needed for creating dependency tree of commands needed for their execution. While stack plays supporting role for this, queue creates the main tree of commands that needs to be executed
struct queue{
	char *data[stack_length];
	int front;
	int rear;
	int size;
};

//Function to add the data to queue 
void add(struct queue *tree, char *value){

	if(tree->size >= stack_length-1){
		return;
	}
	
	//Adding data to the queue until size limit is reached
	if(tree->size == 0){
		tree->data[0] = value;
		tree->front = 0;
		tree->rear = 0;
		tree->size = 1;
	} else{
		tree->rear++;
		tree->data[tree->rear] = value;
		tree->size++;
	}
}

//Function to remove data from the queue
char *remv(struct queue *tree){
	
	char *temp = tree->data[tree->front];
	
	//If queue is not empty it removes th data from the front using First in First Out format
	if (tree->size == 0){
		return NULL;
	} else{
		tree->size--;
		tree->front++;
	}
	
	return temp;
}

//Main function for running of the program
int main(int argc, char **argv){

	//Set the buffer for stdout to obtain proper result in the output
	setbuf(stdout, NULL);
	
	//Open the main fakefile that needs to be parsed and execute it like makefile
	FILE *file = fopen("fakefile", "r");
	
	//If no file is found, exit from the program
	if(file == NULL){
		fprintf(stderr, "./fake: cannot open (fakefile): No such file or directory\n");
		exit(1);
	}
	
	char *buf = NULL;
	size_t bufsize = 0;
	ssize_t eof;
	
	num_recipes = 0;
	
	int i = 0;
	int j = 0;
	char *address[num_L];
	
	//Reading the file line by line to parse the required data
	while ( (eof = getline(&buf, &bufsize, file) ) >= 0 ){
		
		//Ignore the new lines present in the file
		if(buf[0] == '\n'){
			continue;
		}
		
		//Ignore any lines that start with # since they are comments
		if(buf[0] == '#'){
			continue;
		}
		
		//Parse tha target data of the file using the recipes structure defined and save them in respective target
		if(buf[0] != '\t'){
			address[j] = strdup(buf);
			num_recipes++;
			
			//Allocating the memory for the recipes of the fakefile
			recipes = realloc(recipes, sizeof(struct recipe*) * num_recipes);
			
			recipes[i] = malloc(sizeof(struct recipe));
			char *stringp = address[j];
			
			//Stores the target data of recipes
			recipes[i]->target = strsep(&stringp, ":");
			
			if (stringp[0] != '\n'){
				stringp++;
			}
			
			int num_deps = 0;
			int nD = 0;
			recipes[i]->deps = NULL;
			
			//Parse the required dependencies or prerequisites of each target (if any) and save them in deps of recipes
			while(stringp[0] != '\n' && strlen(stringp) != 0){
				num_deps++;
				recipes[i]->deps = realloc(recipes[i]->deps, sizeof(char *) * num_deps);
				recipes[i]->deps[nD] = malloc(sizeof(char));
				
				recipes[i]->deps[nD] = strsep(&stringp, " \n");
				nD++;
				if (stringp == NULL){
					break;
				}
			}
			d[i] = nD;	
			
			j++;
			
			int num_commands = 0;
			int nC = 0;
			recipes[i]->command = NULL;
			
			//Finally, parse the commands for respective target line by line (if there is any) and store them in command of recipes. Each commands starts with tab character which are ignored to parse each command lines along with thier arguments 
			while(1){
				ssize_t ef = getline(&buf, &bufsize, file);
				if(buf[0] == '\n' || ef < 0){
					break;
				}
				address[j] = strdup(buf);
				char *string = address[j];
				string++;
				if (string[strlen(string)-1] == '\n'){
					string[strlen(string)-1] = '\0';
				}
				if (string[strlen(string)] == '\n'){
					string[strlen(string)] = '\0';
				}
				
				num_commands++;
				recipes[i]->command = realloc(recipes[i]->command, sizeof(char *) * num_commands);
				recipes[i]->command[nC] = malloc(sizeof(char));

				recipes[i]->command[nC] = string;
				nC++;
				j++;
			}
			c[i] = nC;
			i++;
		}
		
	}
	
	//Finnaly, file parsing is done and each of targets, their dependencies and respective commands are saved in the recipes structure.
	
	//If no recipes are found, then the fakefile is empty or does not contain any specific target to be executed. So, return from the program.
	if (num_recipes == 0) {
		fprintf(stderr, "./fake: empty file!\n");
		return 0;
	}
	
	//File parsing is done and now stacks and queues are initialized for the creation of dependency tree
	
	s1.top = EMPTY;
	s2.top = EMPTY;
	
	myTree.front = myTree.rear = -1;
	myTree.size = 0;
	
	//If the target is provided by the user in command line, then that target should be executed instead of the deafult one
	if(argv[1] == NULL){
		
		//Create dependency tree based on default target using createTree function
		createTree(recipes[0]->target, 0);
	} else{
		for(int p = 0; p < num_recipes; p++){
			if(strcmp(recipes[p]->target, argv[1]) == 0){
				
				//Create dependency tree based on given specified target using createTree function
				createTree(recipes[p]->target, p);
				break;
			} 
		}
	}
	
	//Dependency tree is created using respective function, now the file is closed and pcommand structure is initialized to store the respective commands lines to be executed
	
	fclose(file);
	
	struct pcommand *pcommands = NULL;
	
	//Alocating memory to store the commands line arguments
	pcommands = realloc(pcommands, sizeof(struct pcommand) * num_L);
	
	int final = 0;
	int toggle = 0;
	int tag = 0;
	int ptag = 0;
	int tD = 0;
	int aD = 0;
	char *tData[myTree.size];
	char *aData[num_L];
	
	//Initilization of respective commands pointers to be parsed using pipes or input/output redirection for the execution of respective commands
	char *rcmd[] = {NULL, NULL, NULL, NULL, NULL, NULL};
	char **cmd[] = {NULL, NULL, NULL, NULL, NULL, NULL,
					NULL, NULL, NULL, NULL, NULL, NULL,
					NULL, NULL, NULL, NULL, NULL, NULL,
					NULL, NULL, NULL, NULL, NULL, NULL,
					NULL, NULL, NULL, NULL, NULL, NULL,
					NULL, NULL, NULL, NULL, NULL, NULL,
					NULL, NULL, NULL, NULL, NULL, NULL,
					NULL, NULL, NULL, NULL, NULL, NULL,
					NULL, NULL, NULL, NULL, NULL, NULL,
					NULL, NULL, NULL, NULL, NULL, NULL,
					NULL, NULL, NULL, NULL, NULL, NULL,
					NULL, NULL, NULL, NULL, NULL, NULL,
					NULL, NULL, NULL, NULL, NULL, NULL};
	
	//Parse the command lines from the dependency tree
	while(1){
		
		if(myTree.size == 0){
			break;
		}
		
		int n = 0;
		cmd[final] =  malloc(sizeof(char **) * 20);
		
		//Remove each command line from the tree and store in the variable for command parsing
		tData[tD] = strdup(remv(&myTree));
		char *string = tData[tD];
		
		if(string[0] == 'g' && string[1] == 'c' && string[2] == 'c'){
			
		} else{
			printf("%s\n", string);
		}
		
		//Each command line is saved in the pcommand structure parsing using '|'
		while(string != NULL){
		
			char *val = strsep(&string, "|");
			if(string != NULL){
				string++;	
			}
			pcommands[n].command = val;
			n++;
		}
		
		int pt = 0;
		
		//Finnaly, each command line can have mutiple sub-commands that use pipes or commands for input/output redirection and their paths
		for(int p = 0; p < n; p++){
			
			int m = 0;
			aData[aD] = strdup(pcommands[p].command);
			char *str = aData[aD];
			aD++;
			
			//Each sub-commands are treated as single command argument and saved in comm defined in pcommand structure
			pcommands[p].comm[m] = strsep(&str, " ");
			
			if (p > 0 && str != NULL && str[0] == ' '){
				strsep(&str, " ");
			} else {
				if (str != NULL && str[0] == '\0'){
					strsep(&str, " ");
				}
			}
			m++;	
			
			//If the command line contains only command with no arguments, store the command and continue parsing
			if(str == NULL){
				
				pt = p;
				pcommands[p].comm[m] = NULL;
				cmd[p] = pcommands[p].comm;
				continue;
			}
			
			//If the command line contains some options that start with '-', store them and continue parsing
			if(str[0] == '-'){
				pcommands[p].comm[m] = strsep(&str, " ");
				m++;
				if(str == NULL) {
					
					pt = p;
					pcommands[p].comm[m] = NULL;
					cmd[p] = pcommands[p].comm;
					continue;
				}
				
			} 
			
			//If the argument of command line starts with inverted commas, then parse accordingly to get the correct argument, store them and continue parsing
			if(str[0] == '\"'){
				str++;
				
				pcommands[p].comm[m] = strsep(&str, "\"");
				
				int s = 0;
				int tg = 0;
				for (int r = 0;  r < strlen(pcommands[p].comm[m]); r++){
					
					if(pcommands[p].comm[m][r] == '\\'){
						
						if(pcommands[p].comm[m][r+1]  == '\\'){
							
							if(pcommands[p].comm[m][r+2]  == 'n'){
								tg = 1;
								continue;
							}
						}
					}
					
					pcommands[p].comm[m][s] = pcommands[p].comm[m][r];
					s++;
				}
				if (tg == 1){
					tg = 0;
					pcommands[p].comm[m][s] = '\0';
				}
				
				strsep(&str, " ");
				
				if ((str != NULL) && (str[0] != '<' && str[0] != '>')){
					strsep(&str, " ");
				}
				m++;
				
				if(str == NULL) {

					pt = p;
					pcommands[p].comm[m] = NULL;
					cmd[p] = pcommands[p].comm;
					continue;
				}
				
			//Finally, for other arguments that does not include input/output redirection, parse the commands line arguments, store them in the memory pointed by comm pointer and continue	
			} else if (str[0] != '<' && str[0] != '>'){
				
				pcommands[p].comm[m] = strsep(&str, " ");
				m++;
				while(str != NULL){
					if(str[0] == '<' || str[0] == '>'){
						break;
					}
					pcommands[p].comm[m] = strsep(&str, " ");
					m++;
				}
				
				if(str == NULL){
					
					pt = p;
					pcommands[p].comm[m] = NULL;
					cmd[p] = pcommands[p].comm;
					continue;
				}
			}
			
			//For input redirection, parse the respective commands and execute the command by sending them using cmd pointer to redirect funtion
			if(str[0] == '<'){
				
				tag = 1;
				toggle = 1;;
				strsep(&str, " ");
				
				pcommands[p].comm[m] = strsep(&str, " ");
				m++;
				
				for(int r = 0; r < m; r++){
					
					rcmd[r] = pcommands[p].comm[r]; 
				}
				
				if(str[0] != '>'){
					
					//Redirect function that parse the command and finally executes the respective command lines that include input/output redirection
					redirect(rcmd);
				}
				
				if(str == NULL){
					break;
				}
				
			}
			
			//For output redirection, parse the respective commands and execute the command by sending them using cmd pointer to redirect funtion
			if(str[0] == '>'){
				
				tag = 1;
				strsep(&str, " ");
				
				pcommands[p].comm[m] = strsep(&str, " ");
				m++;
				
				if (toggle == 1){
					 for(int r = 2; r <= m; r++){
					 	
						rcmd[r] = pcommands[p].comm[r]; 
					}
				} else{
					for(int r = 1; r <= m; r++){
						
						rcmd[r] = pcommands[p].comm[r-1]; 
					}
				}
				toggle = 0;
				
				//Redirect function that parse the command and finally executes the respective command lines that include input/output redirection	
				redirect(rcmd);
				
				if(str == NULL){
					break;
				}
				
			}
		}
		
		pt++;
		cmd[pt] = NULL;
		
		//For those commands that does not include input/output redirection but can include pipes or exists standalone, use pipeline function sending the pointer using cmd
		if(tag == 0){
			 
			if ( strcmp(cmd[0][0], "gcc") == 0){
				int a = 0;
				while(cmd[0][a] != NULL){
					if( ( strcmp(cmd[0][1], "-E") == 0 || strcmp(cmd[0][1], "-c") == 0 ) && access(cmd[0][4],F_OK) == 0 ){
						ptag = 1;
						a++;
						continue;
					}
					
					if(cmd[0][a+1] == NULL){
						printf("%s", cmd[0][a]);
					} else{
						printf("%s ", cmd[0][a]);
					}
					a++;
				}
					
				if(ptag == 0){
					printf("\n"); 
				} else{
					ptag = 0;
				}
			}
			
			//Pipeline function that takes each command line either they are standalone or they are in pipes, parse and execute them accordingly to provide the correct output to the user
			pipeline(cmd);
		} else {
			tag = 0;
		}
		tD++;
	}
	
	//For each command line, the respctive commands, options, arguments alongw ith redirection and pipes, are parsed and passed to respective funtions for the execution
	
	//Free the required allocated memory space whenever needed
	free(pcommands);
	
	for(int f = 0; f < aD; f++){
		free(aData[f]);
	}
	
	for(int f = 0; f < tD; f++){
		free(tData[f]);
	}
	
	for(int f = 0; f < i; f++){
		free(recipes[f]->command);
	}
	
	for(int f = 0; f < i; f++){
		free(recipes[f]->deps);
	}
	
	for(int f = 0; f < i; f++){
		free(recipes[f]);
	}
	
	for(int f = 0; f < j; f++){
		free(address[f]);
	}
	
	free(recipes);
	free(buf);
	return 0;
}

//For the commands that involve input/outpu redirection, the redirect funtion parsethose command using child and parent process to execute them accordingly
static int redirect(char **cmd){
	
	//Initilization of process ideas for creation of child proc
	pid_t pid;
	pid_t pids[100];
	int i = 0;
	int val = 0;
	
	//If creating ac child process gives an error, exit from the program
	if ((pid = fork()) == -1) {
		perror("fork");
		exit(1);
		
	//In every child process, use execvp to execute each command passed to function	
	} else if (pid == 0) {
		
		//For output redirection
		if (*cmd == NULL){
			
			//Create a copy of file descriptors using dup2 to send to the stdout for each output redirection command. The command might contain several arguments, options or just command, so parse accordingly for execution
			if( ( *(cmd+3) == NULL ) && ( *(cmd+4) == NULL) ){
				
				int fd1 = open(cmd[2], O_WRONLY | O_CREAT);
				val = dup2(fd1, STDOUT_FILENO);
				char *com[] = {cmd[1], NULL};
				execvp(com[0], com);
				
				
			} else if(*(cmd+4) == NULL){
				
				int fd1 = open(cmd[3], O_WRONLY | O_CREAT);
				val = dup2(fd1, STDOUT_FILENO);
				char *com[] = {cmd[1], cmd[2], NULL};
				execvp(com[0], com);
			} else {
				
				int fd1 = open(cmd[4], O_WRONLY | O_CREAT);
				val = dup2(fd1, STDOUT_FILENO);
				char *com[] = {cmd[1], cmd[2], cmd[3], NULL};
				
				//Use of execvp along with the respective command line arguments for execution of those commands accordingly
				execvp(com[0], com);
			}
			
			close(val);
			return val;
		
		//For input redirection	
		} else if (*(cmd+2) == NULL) {
		
			//Creates a copy of file descriptors using dup2 to receive the data from stdin to be redirected using input redirection command.
			int fd2 = open(cmd[1], O_RDONLY);
		
			if (fd2 == -1){
				return -1;
			} else{
				val = dup2(fd2, STDIN_FILENO);
			
				char *com[] = {cmd[0], NULL};
				
				//Use of execvp along with the respective command line arguments for execution of those commands accordingly
				execvp(com[0], com);
			}
			close(fd2);
			return val;
		
		//For commands that contain both input and output redirection	
		} else{
			
			//Creates a copy of file descriptors using dup2 to receive the data from stdin to be redirected using input redirection command and then uses output redirection command to send the data to stdout
			int fd1 = open(cmd[2], O_WRONLY | O_CREAT);
			val = dup2(fd1, STDOUT_FILENO);
			
			int fd2 = open(cmd[1], O_RDONLY);
		
			if (fd2 == -1){
				
			} else{
				dup2(fd2, STDIN_FILENO);
			
				char *com[] = {cmd[0], NULL};
				
				//Use of execvp along with the respective command line arguments for execution of those commands accordingly
				execvp(com[0], com);
			}
			
			close(fd2);
			close(val);
			return val;
		}
		
	} else {
			
		pids[i++] = pid;
	}

	//Wait for all the process ids to join all the child process created using the parent process
	for (int j = (i - 1); j > -1; --j) {
		waitpid(pids[j], NULL, 0);
	}
	
	return 0;
}

//Function that creates dependency tree utilizing the queu data structure in which stack plays the supporting role as well
void createTree(const char *name, int index){

	//Parse the respective index that the tree goes through and store them in the stack s2
	char temp[10];
	sprintf(temp, "%d", index);
	push(&s2, temp);
	
	//Push and store respective targets that tree goes through 
	push(&s1, recipes[index]->target);

	//Recursively create dependency tree going through each target based on their dependencies/prerequisites
	ks1[index] = 0;
	for(int i = 0; i < num_recipes; i++){
		
		while(ks1[index] == d[index]){
			
			for(int j = 0; j < c[index]; j++){
				
				if(s2.top != EMPTY){
					
					//Add each command line argument to the main myTree based on the targets and dependencies
					add(&myTree, recipes[index]->command[j]);
				}
			}
			
			pop(&s2);
			if(s2.top == EMPTY){
				return;
			}
			
			index = atoi(s2.data[s2.top]);
			ks1[index]++;
		}
		
		if( strcmp(recipes[index]->deps[ks1[index]], recipes[i]->target) == 0 ){
			
			//Recursively create the tree based on which target depends on which another target needs to be fulfilled first
			createTree(recipes[index]->deps[ks1[index]], i);
		} else{
			
			if (i == (num_recipes-1) ){
				i = 0;
				ks1[index]++;
			} 
			continue;
		}
	}
	
	//For any indepepent targets create the tree by saving their respective command line arguments as well
	if(num_recipes != size(&s1)){	
		
		for(int m = 0; m < num_recipes; m++){
			int toggle = 0;
			for(int n = 1; n <= size(&s1); n++){
					
				if ( strcmp(recipes[m]->target, s1.data[n]) == 0){
					toggle = 0;
					break;
				} 
				toggle = 1;
			}
			
			if (toggle == 1){
				createTree(recipes[m]->target, m);
			}
		}
	}
	
	//Basically, stack s1 stores each target through which the dependency tree passes recursively. It even helps to traverse back whenever needed
	//Stack s2 stores each index for the respective target that helps to queue to create the tree by traversing back and forth the tree whever recursion needs to be used
	//Finally, queue myTree stores all the command line arguments by recu traversing through all the recipes
	return;
}

//Pipeline function to execute the command lines
static void pipeline(char ***cmd){
	
	//Initilization of arrays and variables to hold pipe file descriptors, process id of child process and other file decriptos as well
	
	int fd[2];
	pid_t pid;
	
	//Holds the previous iteration file descriptor
	int fdd = 0;
	pid_t pids[100];
	int i = 0;
	
	if( (strcmp( (*cmd)[0], "false" ) == 0) ){
		exit(1);
	}
	
	while (*cmd != NULL) {
		
		//If this is not the final command, create pipes
		if (*(cmd + 1) != NULL) {
			
			//Create a pipe using the file decriptor parsing and executing commands
			pipe(fd);
		}
		
		//Using fork to split this process into two creating the child process unless the error occurs that forces to exit the program
		if ((pid = fork()) == -1) {
			perror("fork");
			exit(1);
		}

		//Execution of commands in each child process
		else if (pid == 0) {
			
			//Using dup2 to create the file descriptors using the previously create pipe as stdin or this can already be stdin as well
			dup2(fdd, 0);
			
			//Close fdd after it is passed as stdin
			if (fdd > 0) {
				int err = close(fdd);
				if (err) {
					perror("close()");
				}
			}
			
			//If the command lines are in pipes, then make end of current pipe as stdout for another command using dup2 if another command is present
			if (*(cmd + 1) != NULL) {
				dup2(fd[1], 1);
			}
			
			//Close pipe file descriptors for child process
			//In the pipeline, each command goes as stdin for another command and another command goes as stdout which becomes stding for third and the process continues as long as the commands are present
			close(fd[0]);
			close(fd[1]);
			
			//Execute the respective command using execv passing respective command line and their arguments
			
			execvp((*cmd)[0], *cmd);
			
			exit(1);
		
		//Parent process that loops though the process ids and new and old file desciptors, along with running for next command line
		} else {
			
			if (fdd > 0) {
				close(fdd);
			}
			
			pids[i++] = pid;
			close(fd[1]);
			fdd = fd[0];
			
			//Move towards the next command
			cmd++;
		}
	}

	//Wait for all the process ids to of child procesess which is similar like joining the threads
	for (int j = (i - 1); j > -1; --j) {
		waitpid(pids[j], NULL, 0);
	}
}
