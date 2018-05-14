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

#include <sys/stat.h>  // for getting the file size
#include <pwd.h>
#include <grp.h>

#include <stdbool.h>  // for bool type

#include "../errlib.h"
#include "../sockwrap.h"


#define BUFLEN 255

#define MSG_OK  "+OK"
#define MSG_OK_LEN  strlen(MSG_OK)

#define MSG_ERR  "-ERR\r\n"


#ifdef TRACE
#define trace(x) x
#else
#define trace(x)
#endif


char *prog_name;


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


int isOK(char *buf)
{
	char subbuff[4];
	memcpy(subbuff, buf, 3);
	subbuff[4] = '\0';
	// printf("isGET() says: %s\n", subbuff);
	// printf("isGET() result: %d\n", (!strcmp(subbuff, "GET ")));
	return (!strcmp(subbuff, MSG_OK));
}

int isERR(char *buf)
{
	return (!strcmp(buf, "-ERR"));
}

int main (int argc, char *argv[])
{
	char buf[BUFLEN];  /* transmission buffer */
    char rbuf[BUFLEN]; /* reception buffer */

    uint16_t tport_n, tport_h; /* server port number (net/host ord) */

    int sfd;  // socket file descriptor
    int result;
    struct sockaddr_in saddr; /* server address structure */
    struct in_addr sIPaddr;   /* server IP addr. structure */

	/* for errlib to know the program name */
	prog_name = argv[0];


	/* check arguments */
	if (argc < 4)
		err_quit ("usage: %s <serv_addr> <port> <file_name> [<filenames>]\n where <port> can be any number between 2000..65535", prog_name);


	//host = argv[1];
    result = inet_aton(argv[1], &sIPaddr);
	if (!result)
        err_quit("(%s) - Invalid address", prog_name);


	// chec if the port given as argument is a number
	if (isNumber(argv[2]))
		tport_h=atoi(argv[2]);
	else
		err_quit ("error: %s the provided port: '%s' is not a number, use a port number in range 2000..65535\n", prog_name, argv[1]);




	short nr_of_files = (argc - 3);
	char* file_names[nr_of_files];


	for (short i=0; i<nr_of_files; i++){
		file_names[i] = argv[i+3];
		printf("argv[%d] = %s\n", i+3, file_names[i]);
	}


	//fname = argv[3];

	/* create socket */
	//client_socketfd = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	return 0;
}
