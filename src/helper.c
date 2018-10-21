#include "helper.h"

int isInt(char* str) {
	int len = strlen(str);

	if (len == 0 || str == NULL) {
		return 0;
	}

	for(int i = 0; i < len; i++) {
		if (!isdigit(str[i]))
			return 0;
	}
	return 1;
}

//Create a queue and initilize its mutex and condition variable
MessageQueue* createMessageQueue()
{
    MessageQueue* q = (MessageQueue*)malloc(sizeof(MessageQueue));
    q->head = q->tail = NULL;
    pthread_mutex_init(&q->mutex, NULL);
    
    //Initialize the condition variable
    pthread_cond_init(&q->cond, NULL);
    return q;
}

//"Send" a message - append it onto the queue
void sendMessage(MessageQueue* q, char* filename, int sender)
{
    MessageNode* node = (MessageNode*)malloc(sizeof(MessageNode));
    node->msg.sender = sender;
	node->msg.filename = malloc(sizeof(char) * (strlen(filename) + 1));
    strcpy(node->msg.filename, filename);
    node->next = NULL;

    // critical section
    pthread_mutex_lock(&q->mutex);
    if (q->tail != NULL) {
        q->tail->next = node;       // append after tail
        q->tail = node;
    } else {
        q->tail = q->head = node;   // first node
    }
    //Signal the consumer thread woiting on this condition variable
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
    fprintf(stderr, "Worker %d enqueues the message, signals cond variable, unlocks mutex, and goes to sleep\n", sender);
    sleep(2);
}

//"Receive" a message - remove it from the queue
int getMessage(MessageQueue* q, Message* msg_out)
{
    int success = 0;
    
    // critical section
    pthread_mutex_lock(&q->mutex);
    
    //Wait for a signal telling us that there's something on the queue
    //If we get woken up but the queue is still null, we go back to sleep
    while(q->head == NULL){
        fprintf(stderr,"Message queue is empty and getMessage goes to sleep (pthread_cond_wait)\n");
        pthread_cond_wait(&q->cond, &q->mutex);
        fprintf(stderr,"getMessage is woken up - someone signalled the condition variable\n");
    }
    
    //By the time we get here, we know q->head is not null, so it's all good
    MessageNode* oldHead = q->head;
    *msg_out = oldHead->msg;    // copy out the message
    q->head = oldHead->next;
    if (q->head == NULL) {
        q->tail = NULL;         // last node removed
    }
    free(oldHead);
    success = 1;
    
    //Release lock
    pthread_mutex_unlock(&q->mutex);

    return success;
}