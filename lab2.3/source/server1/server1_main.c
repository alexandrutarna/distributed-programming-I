/*
 * TEMPLATE 
 */
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../errlib.h"
#include "../sockwrap.h"

#define LISTENQ 15
#define MAXBUFL 255


#define MSG_ERR "wrong command\r\n"
#define MSG_FILE_NOT_FOUND "File not found on server\r\n"


#ifdef TRACE
#define trace(x) x
#else
#define trace(x)
#endif


char *prog_name;


	/* Checks if the content of buffer buf equals the "GET "*/
int isGET(char *buf)
{
	char subbuff[5];
	memcpy(subbuff, buf, 4);
	subbuff[4] = '\0';
	printf("substring is: %s\n", subbuff);
	return (!strcmp(subbuff, "GET "));
}


char * getFileName(char *buf, int buflen){

	char fileName[64] = "";
	char *myBuff = buf;
	//myBuff[buflen-2] = '\0';
    memcpy(fileName, &myBuff[4], buflen-6);
	
	printf("Filename is:-%s\n", fileName);
    
	char *fileToReturn = fileName;
	return fileToReturn;
}

	/* Checks if the content of buffer buf equals the "QUIT\r\n" */
int isQUIT(char *buf)
{
	return (!strcmp(buf, "QUIT\r\n"));
}


int process_client_request(int connfd){
	char buf[MAXBUFL+1]; /* +1 to make room for \0 */
	char *command;
    char *file_name;

	uint32_t res = 1000;
	int nread;


		while (1) {
		trace( err_msg("(%s) - waiting for command ...",prog_name) );
		/* Do not use Readline as it exits on errors
		(e.g., clients resetting the connection due to close() on sockets with data yet to read).
		servers must be robust and never close */
		/* Do not use Readline_unbuffered because if it fails, it exits! */
		nread = readline_unbuffered (connfd, buf, MAXBUFL);
		if (nread == 0) {
			return 0;
		} else if (nread < 0) {
			err_ret ("(%s) error - readline() failed", prog_name);
			/* return to the caller to wait for a new connection */
			return 0;
		}

		/* append the string terminator after CR-LF that is, \r\n (0x0d,0x0a) */
		buf[nread]='\0';
		trace( err_msg("(%s) --- received string '%s'",prog_name, buf) );


		if (nread >=6) {

			if (isGET(buf)){
				printf("GET command received\n");
				file_name = getFileName(buf, nread);
			}

			else if (isQUIT(buf)){
				printf("QUIT command received\n");
			}

			else {
				printf("send error to the client and close\n");
			}

		}


		/* get the command and send MSG_ERR in case of error */
		if (sscanf(buf,"%s ",&command) != 2) {
			trace( err_msg("(%s) --- wrong or missing operands",prog_name) );
			int len = strlen(MSG_ERR);
			int ret_send = send(connfd, MSG_ERR, len, MSG_NOSIGNAL);
			if (ret_send!=len) {
				printf("cannot send MSG_ERR\n");
				trace( err_msg("(%s) - cannot send MSG_ERR",prog_name) );
			}
			continue;
		}
		trace( err_msg("(%s) --- command %s ",prog_name,command) );
		trace( err_msg("(%s) --- file_name %s ",prog_name,file_name) );

		/* do the operation */
		// res = op1 + op2;


		trace( err_msg("(%s) --- result of the sum: %lu", prog_name, res) );

		/* convert the result to a string */
		snprintf (buf, MAXBUFL, "%u\r\n", res);

		/* send the result */
		int len = strlen(buf);
		int ret_send = sendn(connfd, buf, len, MSG_NOSIGNAL);
		if (ret_send!=len) {
			trace( err_msg("(%s) - cannot send the answer",prog_name) );
		} else {
			trace( err_msg("(%s) --- result just sent back", prog_name) );
		}
	}
}






int main (int argc, char *argv[])
{

	int listenfd, connfd, err=0;
	short port;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t cliaddrlen = sizeof(cliaddr);

	/* for errlib to know the program name */
	prog_name = argv[0];


	/* check arguments */
	if (argc!=2)
		err_quit ("usage: %s <port>\n where <port> can be 0..65535", prog_name);
	port=atoi(argv[1]);
    printf("port=%d\n", port);

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

	while (1) {
		trace( err_msg ("(%s) waiting for connections ...", prog_name) );

		int retry = 0;
		do {
			connfd = accept (listenfd, (SA*) &cliaddr, &cliaddrlen);
			if (connfd<0) {
				if (INTERRUPTED_BY_SIGNAL ||
				    errno == EPROTO || errno == ECONNABORTED ||
				    errno == EMFILE || errno == ENFILE ||
				    errno == ENOBUFS || errno == ENOMEM	) {
					retry = 1;
					err_ret ("(%s) error - accept() failed", prog_name);
				} else {
					err_ret ("(%s) error - accept() failed", prog_name);
					return 1;
				}
			} else {
				trace ( err_msg("(%s) - new connection from client %s:%u", prog_name, inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port)) );
				retry = 0;
			}
		} while (retry);

		err = process_client_request(connfd);

		//Close (connfd);
		trace( err_msg ("(%s) - connection closed by %s", prog_name, (err==0)?"client":"server") );

	}

	return 0;
}