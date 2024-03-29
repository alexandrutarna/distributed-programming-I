/*
 * TEMPLATE 
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <errno.h>
#include <ctype.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <string.h>
#include <time.h>

#include <sys/stat.h>  // for getting the file size

#include <stdbool.h>  // for bool type

#include "../errlib.h"
#include "../sockwrap.h"

#include <signal.h> // added for handling child signals
#include <wait.h>

#define LISTENQ 15
#define MAXBUFL 255

#define MSG_ERR  "-ERR\r\n"
#define MSG_OK  "+OK\r\n"
#define MSG_ERR_LEN  strlen(MSG_ERR)
#define MSG_OK_LEN  strlen(MSG_OK)

#define VALID 6
#define MAX_CHILDREN 3


#ifdef TRACE
#define trace(x) x
#else
#define trace(x)
#endif


char *prog_name;
char child_prog_name[255];

// int pids[3][2];



	/* Checks if the content of buffer buf equals the "GET "*/
int isGET(char *buf)
{
	char subbuff[5];
	memcpy(subbuff, buf, 4);
	subbuff[4] = '\0';
	// printf("isGET() says: %s\n", subbuff);
	// printf("isGET() result: %d\n", (!strcmp(subbuff, "GET ")));
	return (!strcmp(subbuff, "GET "));
}

	/* Checks if the content of buffer buf equals the "QUIT\r\n" */
int isQUIT(char *buf)
{
	char subbuff[5];
	memcpy(subbuff, buf, 4);
	subbuff[4] = '\0';
	// printf("isQUIT() say : %s\n", subbuff);
	// printf("isQUIT() result : %d\n", (!strcmp(buf, "QUIT")));
	return (!strcmp(buf, "QUIT"));
}


bool isNumber(char number[])
	{
   		int i = 0;
   		for (i = 0; number[i] != 0; i++){
       		//if (number[i] > '9' || number[i] < '0')
       		if (!isdigit(number[i]))
           		return false;
   		}
   		return true;
	}


int process_client_request(int connfd){
	char buf[MAXBUFL+1]; /* +1 to make room for \0 */
    char file_name[255];
	int status = 0;
	const int SIZE_OF_CHAR = sizeof(char);

		while (1) {
			trace( err_msg("(%s) - waiting for command ...",prog_name) );

			int nread = 0; 
			char input_character;
			
			do {
				int n = Read( connfd, &input_character, SIZE_OF_CHAR);
				if (n == 1)
				{
					buf[nread] = input_character;
					nread += 1;
				}else 
				 	break;
			} while (nread < MAXBUFL-1 && input_character != '\n');

			if (nread == 0){
				return 0;
			}

		// add '\0' after '\r' and '\n' 
		buf[nread]='\0';
		while (nread > 0 && (buf[nread-1]=='\r' || buf[nread-1]=='\n')) {
			buf[nread-1]='\0';
			nread--;
		}
		trace( err_msg("(%s) --- received string '%s'",prog_name, buf) );

		/*parce the input to figure out the command 
		* 
		* possible combinations: "GET \r\n", "QUIT\r\n" or unknown
		* 	len "GET \r\n" = 6 
		* 	len "QUIT\r\n" = 6
		* 	len unknown < 6, or not starting with "GET \r\n" or "QUIT\r\n"
		*/
		// if (nread >= VALID) {

			if (isGET(buf)){
				strcpy(file_name, buf+4);

				trace( err_msg("(%s) --- requested file by the client is: '%s'\n" , prog_name, file_name) );

				struct stat file_stat;

				// On success, zero is returned.  On error, -1 is returned, and errno is
       			// set appropriately.
				int result = stat(file_name, &file_stat);
				
				if (result == 0) {
					// file exists and the stats are loaded
					FILE *fp;
					fp=fopen(file_name, "rb");

					if (fp != NULL) {
						// file was oppened with success
						int file_size = file_stat.st_size;
						int last_mdf = file_stat.st_mtim.tv_sec;

						// MSG_OK = "+OK\r\n" 
						// MSG_OK_LEN = 5
						Write (connfd, MSG_OK, MSG_OK_LEN );
						trace( err_msg("(%s) --- sent '%s' to client",prog_name, MSG_OK) );

						// converted size of the file in network order
						uint32_t file_size_nw_order = htonl(file_size);

						// send the size of the file in network order
						Write (connfd, &file_size_nw_order, sizeof(file_size_nw_order));
						trace( err_msg("(%s) --- sent size '%d' - converted in network order -> '%d' - to client",prog_name, file_size, file_size_nw_order) );
						
						// convert time of the last modification in network order
						uint32_t last_mdf_nw_order = htonl(last_mdf);

						// send the size of the file in network order
						Write (connfd, &last_mdf_nw_order, sizeof(last_mdf_nw_order));
						trace( err_msg("(%s) --- sent modif time '%d' - converted in network order -> '%d' - to client",prog_name, last_mdf, last_mdf_nw_order) );
						
						int i; char c;
						for (i=0; i<file_size; i++) {
							fread(&c, SIZE_OF_CHAR, 1, fp);
							Write (connfd, &c, SIZE_OF_CHAR);
						}
						trace( err_msg("(%s) --- sent file '%s' to client",prog_name, file_name) );
						fclose(fp);
					}
				}

				// requested file not found or cannot be read
				else {
						trace( err_msg("(%s) --- file '%s' not found - send MSG_ERR and close",prog_name, file_name) );
						Write (connfd, MSG_ERR, MSG_ERR_LEN);
						break;
					}
			
			}	
			else if (isQUIT(buf)){
					trace( err_msg("(%s) --- close request from the client ",prog_name) );
					status = 1;
					break;
				}
				
				// not GET nor QUIT
				else {
				trace( err_msg("(%s) --- UNKNOWN command from the client - send MSG_ERR and close",prog_name) );
				Write (connfd, MSG_ERR, MSG_ERR_LEN);
				// status = 1;
				break;
				}
		}

		// close the socket created for this child
		Close(connfd);
	return status;
}



int main (int argc, char *argv[]){

	int listenfd, new_sockfd, err=0;
	short port;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t cliaddrlen = sizeof(cliaddr);


	pid_t childpid;

	/* for errlib to know the program name */
	prog_name = argv[0];

	/* check arguments */
	if (argc!=2)
		err_quit ("usage: %s <port>\n where <port> can be any number between 2000..65535", prog_name);

	// chec if the port given as argument is a number
	if (isNumber(argv[1]))
		port=atoi(argv[1]);
	else
		err_quit ("error: %s the provided port: '%s' is not a number, use a port number in range 2000..65535\n", prog_name, argv[1]);

	/* create socket */
	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	/* specify address to bind to */
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	Bind(listenfd, (SA*) &servaddr, sizeof(servaddr));

	trace ( err_msg("(%s) socket created",prog_name) );
	trace ( err_msg("(%s) listening on %s:%u", prog_name, inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port)) );

	Listen(listenfd, LISTENQ);

	for (;;){
	
		trace( err_msg ("(%s) waiting for connections ...", prog_name) );

		new_sockfd = Accept (listenfd, (SA*) &cliaddr, &cliaddrlen);
		trace ( err_msg("(%s) - new connection from client %s:%u", prog_name, inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port)) );


		trace ( err_msg("(%s) - fork() to create a child", prog_name) );
		
		childpid = fork();
		if (childpid == -1)
      	trace ( err_msg("(%s) - fork error", prog_name) );
    
    	if (childpid == 0){
			// start child

			int newServerPID = getpid();
			sprintf(child_prog_name, "%s child %d", prog_name, newServerPID);
			prog_name = child_prog_name;

			trace ( err_msg("(%s) ----- CHILD [%d]- STARTED", prog_name, newServerPID) );

			trace ( err_msg("(%s) - CHILD - close listenfd [%d]", prog_name, listenfd) );
			Close(listenfd);

			// call the actual function that handles the client
			err = process_client_request(new_sockfd);
			
			trace( err_msg ("(%s) - CHILD - connection closed by %s",prog_name, (err==1) ? "client":"server") );
			trace ( err_msg("(%s) ----- CHILD [%d]- STOPPED", prog_name, newServerPID) );
			
			// terminate child
			exit(0);
			
		} else {
			// parent
			// ignore termination of the cild, needed for avoiding zombies
			signal(SIGCHLD,SIG_IGN);
			Close(new_sockfd);
			trace ( err_msg("(%s) - PARENT - close new_sockfd [%d]", prog_name, new_sockfd) );
		}
    
	}
	return 0;
}
