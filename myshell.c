/****************************************************************
 * Name        : Thanh Le                                              *
 * Class       : CSC 415                                        *
 * Date        : 10/05/2018                                               *
 * Description :  Writting a simple bash shell program          *
 *                that will execute simple commands. The main   *
 *                goal of the assignment is working with        *
 *                fork, pipes and exec system calls.            *
 ****************************************************************/

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/wait.h>
#include <unistd.h>
#include <pwd.h>


#define BUFFERSIZE 256
#define PROMPT "myShell >> "
#define PROMPTSIZE sizeof(PROMPT)
#define delimiters " \n\0"

//Declare functions
void execute(char**, int);
void executeCommand( char**, int, int);
void executeCommand_W_Pipe(char**, int, int [], int);
char ** newArgv(char**, int, int);

void history(char *[], int);

int getch(void);

int main(int* argc, char** argv)
{
	char str[BUFFERSIZE];
	char *token;
	int status;
	char cwd[BUFFERSIZE];
	int myargc;;
	char *current_path = malloc(128);
	char *new_path = malloc(128);
	int byte_formatted;
	int background;
	int pipe_at;
	//int pipe_list[5];
	char c;
	int type = 0;

	char *homedirectory = getenv("HOME"); // search home directory
	// Extra credit, showing current directory
	if((getcwd(cwd, sizeof(cwd)) != NULL)){
		byte_formatted = sprintf(current_path, "myshell ~%s >> ",cwd);
		}
	write(1, current_path, byte_formatted);

	
	system("/bin/stty raw isig");
	while(c = getc(stdin) != '.'){
	//	type++;
printf("%c\n", c);
if(c == 256 + 72) printf("arrow up\n");
if (c == 256 + 80) printf("arrow downn\n");
	
		//exit(1);

	}
	system("/bin/stty cooked");
	

while(1){
	char **myargv = malloc(BUFFERSIZE);
	int pipe_list[5];
	myargc = 0;	
	pipe_at = 0;
	

if(fgets(str,BUFFERSIZE, stdin) != NULL){
	// Split input string into small sub-strings
	token = strtok(str, delimiters);
	while(token != NULL){
		// check if command line contains a pipeline
		if(strcmp(token, "|") == 0){
			pipe_list[pipe_at++] = myargc;
		//pipe_at = myargc;
		}
		myargv[myargc++] = token;
		token = strtok(NULL, delimiters);
	}
		myargv[myargc] = NULL;

	// type exit to terminate the program
  if(myargc > 0){
	if(strcmp(myargv[0], "exit") == 0){
		exit(EXIT_FAILURE);
	}
	

	// Implement cd command
	else if(strcmp(myargv[0],"cd") == 0){
		if(chdir(myargv[1]) != 0){
			perror("chdir failed");
		}
	}

	// Implement pwd command
	else if(strcmp(myargv[0],"pwd") == 0){
		if((getcwd(cwd, sizeof(cwd)) != NULL))
			printf("%s\n", cwd);
		else
			perror("getcwd() error");		
	}

	// execute commands
	else{
		if(strcmp(myargv[myargc-1], "&") == 0){
		background = 1;
		myargv = newArgv(myargv,0,myargc-1);
		--myargc;
		}

	//	if(sizeof(pipe_list)/sizeof(pipe_list[0]) > 0){
	if(pipe_at > 0){
			executeCommand_W_Pipe(myargv, myargc, pipe_list, background);
		}
		else{
			executeCommand(myargv, myargc, background);
		}
	}


	free(myargv); // Clean the buffer
}
	// Update the current home directory
	byte_formatted = sprintf(current_path,
	 "myshell %s >> ",getcwd(cwd, sizeof(cwd)));

	// Check if users home directory is in curren_path
	// if yes, add symbol ~, otherwise, no
	if(strstr(current_path, homedirectory)){
	byte_formatted = sprintf(current_path,
	 "myshell ~%s >> ",getcwd(cwd, sizeof(cwd)));
	write(1, current_path, byte_formatted);
	}
	else{
	write(1, current_path, byte_formatted);
	}
	
  } // end the expression argv > 0 
} // end while loop
return 0;
}

// Implementing execution function
void execute(char** myargv, int background){

	pid_t pid, wait_pid;
	int status;
	if((pid = fork()) == 0)  // Child process
	{
		if(execvp(myargv[0],myargv) < 0)
		{
		printf("Error execution\n");
		}
		exit(1);
	}
	else if (pid < 0){
		perror("Failed to fork process\n");
		exit(EXIT_FAILURE);
	}	
	else{ // Parent process
		// The parent waits for its child until its process is finished
		// Note: It won't wait if users request for background execution
		if(background != 1){
		if(waitpid(pid, &status, WUNTRACED) < 0){
			perror("Error while waiting process");
		}
	   }
	}
}

// Implement a command with a pipeline
void executeCommand_W_Pipe(char ** argv, int argc, int pipe_list[],int background){
 	//int num_pipe = sizeof(pipe_list) / sizeof(pipe_list[0]);

		pid_t pid1, pid2;
		int status; 
		// Note: mypine[1] is for writing,
		// and   mypine[2] is for reading.
		int mypipe[2];
		char ** arg1 = malloc(BUFFERSIZE);
		char ** arg2 = malloc(BUFFERSIZE);
		int arg1_size = pipe_list[0];
		int arg2_size = argc - (arg1_size + 1);

		// separate two different argvs 
		arg1 = newArgv(argv,0, arg1_size);
		arg2 = newArgv(argv,arg1_size+1, arg2_size);


		// Create a pine named mypine 
		// to communicate between arg1 and arg2
		if(pipe(mypipe) == -1){
			perror("failed to create pipe");
			exit(EXIT_FAILURE);
		}

		// Create first child process for writing to the pipe
		if((pid1 = fork()) == 0){
			dup2(mypipe[1], STDOUT_FILENO); // write output to end of pipe
			close(mypipe[0]); // close unused read pipe
	//		if(execvp(arg1[0], arg1) < 0){
	//			perror("error at child 1 execution\n");
	//		}
			executeCommand(arg1, arg1_size, background);
			exit(1);
		}
		else if (pid1 < 0){
			perror("failed to fork at arg1\n");
		}
		// Parents waits for its child process
		else 
			if(waitpid(pid1,&status, WUNTRACED) < 0)
			perror("Error while waiting process at pid1");
		// Close after writing is finished
		close(mypipe[1]);
		
	// ---------------------------------------------------
		// The second child process is created to read 
		// input from end of pipe
		if((pid2 = fork()) == 0){
			dup2(mypipe[0], STDIN_FILENO); // read input from end of pipe
	//		close(mypipe[1]); // close unused write pipe
	//		if(execvp(arg2[0], arg2) < 0){
	//			perror("error at child 2 execution\n");
	//		}
		
			executeCommand(arg2, arg2_size, background);

			exit(1);
		}
		else if (pid2 < 0){
			perror("failed to fork at arg2\n");
		}

		else 
	//	if(background != 1){
		// Parents waits for its child process
		if(waitpid(pid2, &status, WUNTRACED) < 0)
			perror("Error while waiting process at pid2"); 
	//}
		// Close after reading is finished
		close(mypipe[0]);
		free(arg1);
		free(arg2);
}


void executeCommand(char ** argv, int argc, int background){
int execution = 1;

  // check if there is any another special characters in
  //array arvc[] such as "<", ">>", ">", and "|".
  for(int i = 1; i < argc; i++){
	//Redirect standard output to a file
  	if(strcmp(argv[i],">") == 0){
  		int tmpfd = dup(STDOUT_FILENO); //create temp fd to hold std_out
		int fd_out = open(argv[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0777);
		if(fd_out < 0) {
			printf("failed to open the file %s\n", argv[i+1]);
			close(fd_out); 
			exit(EXIT_FAILURE);
		} 
		// Duplicate fd_out on STDOUT_FILENO
		if(dup2(fd_out, STDOUT_FILENO) < 0){
		perror("can not redirect std output");
		}
	//Execute the command
		execute(newArgv(argv,0,i), background);

		close(fd_out);
		dup2(tmpfd, STDOUT_FILENO);
		close(tmpfd);

		// set executeion to 0, so the command won't be executed again
		execution = 0;
		break;
		}

	// Implement std output append
	else if(strcmp(argv[i],">>") == 0){
		int tempfd = dup(STDOUT_FILENO); // stdout # is 1
		// it can also be written as tempfd = dup(1);
		int fd_out = open(argv[i+1], O_WRONLY | O_CREAT | O_APPEND, 0777);
		if(fd_out < 0){
		printf("failed to open the file %s\n", argv[i+1]);
			close(fd_out); 
			exit(EXIT_FAILURE);
		}
		//Duplicate fd_out on stdout
		if(dup2(fd_out,STDOUT_FILENO) < 0){
		perror("can not redirect std output");
		}

		//Execute the program
		execute(newArgv(argv,0,i), background);

		close(fd_out);
		dup2(tempfd, STDOUT_FILENO);
		close(tempfd);

		// set executeion to 0, so the command won't be executed again
		execution = 0;
		break;
	}	
	
	//Redirect standard input from a file
	else if(strcmp(argv[i],"<") == 0){
		int tmpfd = dup(STDIN_FILENO);
		int fd_in = open(argv[i+1], O_RDONLY);
		if(fd_in < 0){
			printf("failed to open the file %s\n", argv[i+1]);
			close(fd_in);
			exit(EXIT_FAILURE);
		}

		//Duplicate fd_in on stdin
		if(dup2(fd_in, STDIN_FILENO) < 0) {
		perror("can not redirect std input"); }
	
		// execute the command 
		execute(newArgv(argv,0,i), background);
		close(fd_in);
		dup2(tmpfd, STDIN_FILENO);
		close(tmpfd);

	// set executeion to 0, so the command won't be executed again
		execution = 0;
			break;
	}  

   } // End for loop

	// If there is no special characters are found
    // just execute the command normally
	if(execution == 1){	
	execute(argv, background);
	}
}
// Create a new arg command without a special characters
char ** newArgv(char ** myArgv, int begin, int myArgc){
	char ** newMyArgv = malloc(BUFFERSIZE);
	int index = begin;
	for(int i = 0; i < myArgc; i++){
		newMyArgv[i] = myArgv[index++];
	}
	newMyArgv[myArgc] = NULL;
	return newMyArgv;
}
