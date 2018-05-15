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

#include <stdbool.h>  // for bool type

#include "../errlib.h"
#include "../sockwrap.h"

#define BUFLEN 255

#define MSG_OK  "+OK"
#define MSG_OK_LEN  strlen(MSG_OK)

#define MSG_ERR  "-ERR\r\n"
// #define MSG_QUIT  "QUIT\r\n"
// #define MSG_QUIT_LEN  strlen(MSG_QUIT)

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
	subbuff[3] = '\0';
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

    int client_socketfd;  // socket file descriptor
    int result;
    struct sockaddr_in saddr; /* server address structure */
    struct in_addr sIPaddr;   /* server IP addr. structure */

	/* for errlib to know the program name */
	prog_name = argv[0];

	const int SIZE_OF_CHAR = sizeof(char);
	
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

	tport_n = htons(tport_h);

	short nr_of_files = (argc - 3);
	char* file_names[nr_of_files];


	for (short i=0; i<nr_of_files; i++){
		file_names[i] = argv[i+3];
		printf("argv[%d] = %s\n", i+3, file_names[i]);
	}


	//fname = argv[3];

	/* create socket */
	client_socketfd = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    printf("done. Socket fd number: %d\n", client_socketfd);

    /* prepare address structure */
    bzero(&saddr, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = tport_n;
    saddr.sin_addr = sIPaddr;

	trace ( err_msg("(%s) socket created",prog_name) );

    /* connect */
    showAddr("Connecting to target address", &saddr);
    Connect(client_socketfd, (struct sockaddr *)&saddr, sizeof(saddr));
    printf("Connected .\n");

    /* main client loop */
for (int i=0; i<nr_of_files; i++){
	
	char *file_name;
	file_name = argv[i+3];

	printf("filename to download: '%s'\n", file_name);

	char command[255] = "GET ";
	strcat(command, file_name);
	strcat(command, "\r\n");

	printf("--- command: %s", command);
	Write (client_socketfd, command, strlen(command));

	int nread = 0; 
	char input_character;

	int n = 0;
	do {
		n = Read( client_socketfd, &input_character, SIZE_OF_CHAR);
		if (n == 1)
		{
			buf[nread] = input_character;
			nread += 1;
		}else 
		 	break;
	} while (nread < BUFLEN-1 && input_character != '\n');

	// add '\0' after '\r' and '\n' 
	buf[nread]='\0';
	while (nread > 0 && (buf[nread-1]=='\r' || buf[nread-1]=='\n')) {
		buf[nread-1]='\0';
		nread--;
	}

	if (isOK(buf)){
		
		printf("+OK received from server\n");

		char size_nw_order[4];
		uint32_t file_size;

		// read 4 bytes - size og the file in nw order
		n = Read( client_socketfd, &size_nw_order, SIZE_OF_CHAR*4);
		if (n == 4)			
			{
				file_size = ntohl((*(uint32_t *)size_nw_order));
				printf("received size %u\n", file_size);
			}else 
		 		printf("size not received\n");


		char timestamp_nw_order[4];
		uint32_t last_mdf;
		// read 4 bytes - timestamp of the last modification in nw order
		n = Read( client_socketfd, &timestamp_nw_order, SIZE_OF_CHAR*4);
		if (n == 4)
			{
				last_mdf = ntohl((*(uint32_t *)timestamp_nw_order));
				printf("received timestamp %u\n", last_mdf);
			}else 
				printf("last mdf not received\n");



			// char file_buf[255];
			// nread = 0;
			// 		do {
			// 			n = Read( client_socketfd, &input_character, SIZE_OF_CHAR);
			// 			if (n == 1)
			// 			{
			// 				file_buf[nread] = input_character;
			// 				nread += 1;
			// 			}else 
			// 		 	break;
			// 		} while (nread < file_size);

			// 		printf("received content %s\n", file_buf);
		


		char new_file_name[255];
		sprintf(new_file_name, "NEW_%s", file_name);


		FILE *fp;
		if ( (fp=fopen(new_file_name, "wb"))!=NULL) {
			char c;
			int i;
			for (i=0; i<file_size; i++) {
				Read (client_socketfd, &c, SIZE_OF_CHAR);
				fwrite(&c, SIZE_OF_CHAR, 1, fp);
			}
			fclose(fp);
			trace( err_msg("(%s) --- Received file: '%s'",prog_name, new_file_name));
			// trace( err_msg("(%s) \tsize:\t'%s'",prog_name, file_size));
			// trace( err_msg("(%s) \tlast modification:\t'%s'",prog_name, last_mdf));
		}
		// printf("received '%s' from server", buf2);

	} else
		if (isERR(buf)){
			printf("------ ERR received from server\n");
		}


	printf("========================================\n");
		// end of the for 
}

	char *MSG_QUIT = "QUIT\r\n";
	short MSG_QUIT_LEN = strlen(MSG_QUIT);
	Write (client_socketfd, MSG_QUIT, MSG_QUIT_LEN);
	Close (client_socketfd);

	return 0;
}
