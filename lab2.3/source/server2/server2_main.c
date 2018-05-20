/*
 * TEMPLATE 
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


char *prog_name;


int pids[3][2];

int active_children = 0;




void doSomeWork(){
}

int main (int argc, char *argv[])
{
	
int cmd = 0;

printf("PID = %d\n\n", getpid());

	while (1){
		printf("Enter 1 to fork, 0 to do noth\n");
		scanf("%d", &cmd);
		if (cmd == 1){
			printf("fork cmd\n");
			pid_t pid = fork();
			printf("fork returned: %d\n", (int) pid);

			if (pid < 0){
				printf("(%d) - Fork failed\n", (int) getpid());
			}

			if (pid == 0){
				// CHILD
				printf("\t(%d) - I am the child\n", (int) getpid());
				sleep(5);
				exit(0);
			}
		}
		else if (cmd == 0) {
			printf("!fork cmd \n");
		}
		else {
			exit(0);
		}


			// PARENT
	printf("\t(%d) - I am the Parent waiting for child to end\n", (int) getpid());
	int status = 0;
	pid_t child_pid = wait(&status);
	printf("\t- parent knows chuld: %d finished with status: %d\n", (int) child_pid, status);


	int childRetVal = WEXITSTATUS(status);
	printf("\t(%d) - returned value was\n", (int) childRetVal);
	printf("\t(%d) - parent ending\n", (int) getpid());
	}
	
	
	return 0;
}
