#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include "helper.h"

#define NUM_THREADS 10

//Each thread needs multiple arguments, so we create a dedicated struct
typedef struct {
    int workerId;
    MessageQueue* q;
    long consocket;
} ThreadArgs;

void* download(void* arg);
 
int main(int argc, char *argv[]) {
	int currThread = 0;

	pthread_t tid[NUM_THREADS];
	ThreadArgs * args[NUM_THREADS];
  
	struct sockaddr_in dest; // socket info about the machine connecting to us
	struct sockaddr_in serv; // socket info about our server
	int mysocket;            // socket used to listen for incoming connections
	socklen_t socksize = sizeof(struct sockaddr_in);

	MessageQueue * q = createMessageQueue();

	//check number of args
	if (argc < 2) {
		fprintf(stderr, "Not enough arguments\n");
		return 0;
	}

	//check if what *should* be the port num is actually an int
	if (!isInt(argv[1])) {
		fprintf(stderr, "Port number was not an integer\n");
		return 0;
	}

	//convert str arg to int for usage
	int PORTNUM = atoi(argv[1]);

	memset(&serv, 0, sizeof(serv));           // zero the struct before filling the fields
	
	serv.sin_family = AF_INET;                // Use the IPv4 address family
	serv.sin_addr.s_addr = htonl(INADDR_ANY); // Set our address to any interface 
	serv.sin_port = htons(PORTNUM);           // Set the server port number

	/* Create a socket.
   	   The arguments indicate that this is an IPv4, TCP socket
	*/
	mysocket = socket(AF_INET, SOCK_STREAM, 0);
  
	// bind serv information to mysocket
	// Unlike all other function calls in this example, this call to bind()
	//does some basic error handling
	if (bind(mysocket, (struct sockaddr *)&serv, sizeof(struct sockaddr)) != 0){
		fprintf(stderr, "Unable to open TCP socket on localhost:%d\n", PORTNUM);

	// 	//strerror() returns a string representation of the system variable "errno"
	// 	//errno is the integer code of the error that occured during the last system call from this process
	// 	//need to include errno.h to use this function
		// print to stderr
		fprintf(stderr, "%s\n", strerror(errno));
		close(mysocket);
		return 0;
	}

	// start listening, allowing a queue of up to 10 pending connections
	// check if listen failed
	if (listen(mysocket, 10) == -1) {
		fprintf(stderr, "%s\n", strerror(errno));
	}
	
	// Create a socket to communicate with the client that just connected
	long consocket = accept(mysocket, (struct sockaddr *)&dest, &socksize);

	if (consocket == -1) {
		fprintf(stderr, "%s\n", strerror(errno));
	}
  
	while(consocket) {
		printf("Incoming connection from %s\n", inet_ntoa(dest.sin_addr));


		args[currThread] = malloc(sizeof(ThreadArgs));
		args[currThread]->consocket = consocket;
		args[currThread]->q = q;
		args[currThread]->workerId = currThread;

		pthread_create(&tid[currThread], NULL, download, args[currThread]);
		currThread++;

		//keep listening
		consocket = accept(mysocket, (struct sockaddr *)&dest, &socksize);
	}

	close(consocket);
	close(mysocket);
	return EXIT_SUCCESS;
}

/* for reference
typedef struct {
    int workerId;
    MessageQueue* q;
    int consocket;
} ThreadArgs;
*/

void * download(void * arg) {

	//start with a guess that the header won't be larger than 255 bytes
	long MAXRCVLEN = 255;
	char headBuffer[MAXRCVLEN + 1]; //for \0
	char ack[7];

	strcpy(ack, "200");

	ThreadArgs* args = (ThreadArgs*)arg;

	int consocket1 = args->consocket;
	int workerId = args->workerId;

	// Receive data from the client, first chunk received is the "header"
	// header format is "len buffSize numBlocks fName"
	int len = recv(consocket1, headBuffer, MAXRCVLEN, 0);
	if (len == -1) {
		fprintf(stderr, "%s\n", strerror(errno));
	}

	//Send data to client
	send(consocket1, ack, strlen(ack), 0);
	
	headBuffer[len] = '\0';

	//tokenize header by space
	char* lenStr = strtok(headBuffer, " ");
	char* buffSizeStr = strtok(NULL, " ");
	char* numChunksStr = strtok(NULL, " ");
	char* filename = strtok(NULL, "\0");

	char diskFilename[strlen(filename) + 6];
	sprintf(diskFilename, "files/%s", filename);


	//header recv'd, download started, add to queue
	sendMessage(args->q, filename, diskFilename, workerId);

	MAXRCVLEN = atol(buffSizeStr);
	long numChunks = atol(numChunksStr);

	char buffer[MAXRCVLEN + 1]; // +1 so we can add null terminator

	if (MAXRCVLEN == 0 || numChunks == 0) {
		fprintf(stderr, "%s\n", "Header error");
		return NULL;
	}

	FILE* fp = fopen(diskFilename, "w");
	if(!fp) {
		fprintf(stderr, "%s\n", strerror(errno));
	}
	
	//begin to receive and print chunks to file defined by numChunks
	for(int i = 0; i < numChunks; i++) {
		len = recv(consocket1, buffer, MAXRCVLEN, 0);

		if (len == -1) {
			fprintf(stderr, "ERROR: %s\n", strerror(errno));
		}

		buffer[len] = '\0';
		//print buffer data to file
		printf("%s", buffer);
		fprintf(fp, "%s", buffer);

		//Send data to client
		send(consocket1, ack, strlen(ack), 0);
	}

	Message deletedMsg;

	//file completed downloading, remove from current downloads list
	if (!getMessage(args->q, filename, diskFilename, &deletedMsg)) {
		fprintf(stderr, "Error");
	}

	printf("Worker removed message: Filename \"%s\" Disk Filename: \"%s\"\n", deletedMsg.filename, deletedMsg.diskFilename);

	if(deletedMsg.filename) free(deletedMsg.filename);
	if(deletedMsg.diskFilename) free(deletedMsg.diskFilename);

	//close consocket
	close(consocket1);
	fclose(fp);

	printf("\nWorker thread done\n");

	return NULL;
}