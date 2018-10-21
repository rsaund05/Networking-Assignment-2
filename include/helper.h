#ifndef __CZIMMER_HELPERH__

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

#endif

typedef struct {
    char* filename;
    int sender;
} Message;

//Message node
typedef struct message_node {
    Message msg;
    struct message_node* next;
} MessageNode;

//Message queue - a singly linked list
//Remove from head, add to tail
typedef struct {
    MessageNode* head;
    MessageNode* tail;
    pthread_mutex_t mutex;
    
    //Add a condition variable
    pthread_cond_t cond;
} MessageQueue;

int isInt(char* str);
MessageQueue* createMessageQueue();
void sendMessage(MessageQueue* q, char* filename, int sender);
int getMessage(MessageQueue* q, Message* msg_out);