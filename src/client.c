#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/timeb.h>
#include <netdb.h>
#include <errno.h>

#define DEF_BUFF_SIZE 512


//****************************************************************************************************************************************************
int main(int argc, char* argv[]){
	int status, s;
	long len;
	char buff[255];
	char* inBuff;
	char* fname;
	char* pNum;
	const char delim[2] = ":";
	char* tok;
	char* fTok;
	char* ipAddr;
	char* temp;
	int blockSize = DEF_BUFF_SIZE;
	char* text;
	char* msg;
	char* header = malloc(sizeof(char) * 100);
	header[strlen(header) + 1] = '\0';

	double t_diff;
	struct timeb t_start, t_end;  


	int pN;
	char recvMsg[5];

	int receiving;

	int numBlocks;

	int protocol = 0; //0 means IPv4, 1 = IPv6
	int clientSocket;
	char abuf[INET_ADDRSTRLEN];
	struct sockaddr_in* sinp;
	const char* addr;
	struct addrinfo hints, *res, *res0;
	struct sockaddr_in dest;


	FILE* fp;
	if((strstr(argv[1], delim) == NULL)){
		printf("You must provide IPADDRESS:PORTNUMBER FILENAME BUFFERSIZE (Optional) in that format. Exiting\n");
		return 0;
	}

	//Setup for input buffer string
	inBuff = malloc(sizeof(char) * 50);
	inBuff[strlen(inBuff) + 1] = '\0';

	strcpy(inBuff, argv[1]);
	
	//Get ip from commandline
	tok = strtok(inBuff, delim);

	ipAddr = malloc(sizeof(char) * strlen(tok) + 1);
	ipAddr[strlen(ipAddr) + 1] = '\0';
	strcpy(ipAddr, tok);

	printf("IP: %s\n", ipAddr);

	//Get portnumber
	tok = strtok(NULL, delim);

	pNum = malloc(sizeof(char) * strlen(tok) + 1);
	pNum[strlen(pNum) + 1] = '\0';

	strcpy(pNum, tok);
	printf("Port Number: %s\n", pNum);

	//Getting filename
	fname = malloc(sizeof(char) * strlen(argv[2]) + 1);
	fname[strlen(fname) + 1] = '\0';
	strcpy(fname, argv[2]);
	printf("Filename: %s\n", fname);


	//Opening file, retrieving length of contents
	fp = fopen(fname, "r");
	if(fp == NULL){
		printf("Error opening file. Exiting\n");
		free(fname);
		free(ipAddr);
		free(pNum);
		return 0;
	}
	fseek(fp, 0L, SEEK_END);
	
	len = ftell(fp);
	
	//Rewind file pointer to beginning of file
	rewind(fp);

	printf("\nLength of file: %d bytes\n", len);

	//If the user has inputted a specific buffer size
	if(argv[3] != NULL){
		temp = malloc(sizeof(char) * strlen(argv[3]) + 1);
		temp[strlen(temp) + 1] = '\0';
		strcpy(temp, argv[3]);
		blockSize = atoi(temp);
	}
	printf("Block size (bytes): %d\n\n", blockSize);

	//Getting 
	text = malloc(sizeof(char) * len + 1);
	text[strlen(text) + 1] = '\0';
	while(fgets(buff, 100, fp) != NULL){
		strcat(text, buff);
	}
	//****************************************************
	//Done setup, start workin'

	memset (&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	memset(&dest, 0, sizeof(dest));

	pN = atoi(pNum);

	status = getaddrinfo(ipAddr, pNum, &hints, &res);
	if(status != 0){
		printf("GetAddrInfo Error: %d\n", status);
		return 0;
	}

	//Determining if IPv4 or IPv6
	for(res0 = res; res0 != NULL; res0 = res0->ai_next) {
		if(res0->ai_family == AF_INET) {
			sinp = (struct sockaddr_in*)res0->ai_addr;
			addr = inet_ntop(AF_INET, &sinp->sin_addr, abuf, INET_ADDRSTRLEN);
			printf(" address %s", addr?addr:"unknown");
			printf(" port %d", ntohs(sinp->sin_port));
			protocol = 0;
		} else if(res0->ai_family == AF_INET6){
			sinp = (struct sockaddr_in*)res0->ai_addr;
			addr = inet_ntop(AF_INET6, &sinp->sin_addr, abuf, INET6_ADDRSTRLEN);
			printf(" address %s", addr?addr:"unknown");
			printf(" port %d", ntohs(sinp->sin_port));
			protocol = 1;
		}
	}
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = inet_addr(addr);
	dest.sin_port = htons(pN);
	printf("\n");
	if(protocol == 0) {
		clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	} else if(protocol == 1) {
		clientSocket = socket(AF_INET6, SOCK_STREAM, 0);
	} else {
		printf("Unknown error occurred.\n");
		return 0;
	}
	ftime(&t_start);
	if(connect(clientSocket, (struct sockaddr*)&dest, sizeof(struct sockaddr_in)) == -1){
		fprintf(stderr, "ERROR: %s\n", strerror(errno));
		return 0;
	}

	numBlocks = len / blockSize;
	if(len % blockSize != 0) {
		numBlocks++; //If theres a remainder, add another block
	}

	printf("Number of blocks: %d\n", numBlocks);
	//generating and sending header

	sprintf(header, "%ld %d %d %s", len, blockSize, numBlocks, fname); 
	//printf("HEADER: %s\n", header);

	if(send(clientSocket, header, strlen(header), 0) == -1){
		fprintf(stderr, "ERROR: %s\n", strerror(errno));
	}
	receiving = recv(clientSocket, recvMsg, 5, 0);
	

	//Sending the blocks
	
	msg = malloc(sizeof(char) * blockSize + 1);
	memset(msg, '\0', blockSize);

	char* ptr2 = text; //Store beginning of text
	int lenPtr = strlen(ptr2);
	while(lenPtr > blockSize){
		memset(msg, '\0', blockSize);
		strncpy(msg, ptr2, blockSize);
		msg[blockSize] = '\0'; 
		ptr2 += blockSize;
		//printf("MSG: %s\n", msg);
		send(clientSocket, msg, strlen(msg), 0);
		receiving = recv(clientSocket, recvMsg, 5, 0);
		lenPtr = strlen(ptr2);
	}
	memset(msg, '\0', blockSize);
	strcpy(msg, ptr2);
	msg[blockSize] = '\0'; 
	//printf("MSG: %s\n", msg);
	send(clientSocket, msg, strlen(msg), 0);
	receiving = recv(clientSocket, recvMsg, 5, 0);

	ftime(&t_end);

	t_diff = (1000.0 * (t_end.time - t_start.time) + (t_end.millitm - t_start.millitm));
	t_diff = t_diff / 1000.0;
	printf("\nTime to send file (s): %.2lf\n", t_diff);
	printf("File transfer rate (Bytes/sec): %.2lf\n\n", (len/t_diff));

	fclose(fp);
	free(fname);
	free(ipAddr);
	free(pNum);
	// free(temp);
	// free(buff);
	return 0;
}

