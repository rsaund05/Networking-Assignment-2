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
void sendMessage(MessageQueue* q, char* filename, char* diskFilename, int sender)
{
    MessageNode* node = (MessageNode*)malloc(sizeof(MessageNode));
    node->msg.sender = sender;
	node->msg.filename = malloc(sizeof(char) * (strlen(filename) + 1));
	node->msg.diskFilename = malloc(sizeof(char) * (strlen(filename) + 1));
    strcpy(node->msg.filename, filename);
    strcpy(node->msg.diskFilename, diskFilename);
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
    fprintf(stderr, "Worker %d enqueues the message, signals cond variable, unlocks mutex\n", sender);
}

//"Receive" a message by string search - remove it from the queue
int getMessage(MessageQueue* q, char* filename, char* diskFilename, Message* msg_out)
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


    //case 1: the head is the item we need to remove
    if (strcmp(q->head->msg.filename, filename) == 0 && strcmp(q->head->msg.diskFilename, diskFilename) == 0) {
    	MessageNode* oldHead = q->head;
    	*msg_out = oldHead->msg;    // copy out the message
    	q->head = oldHead->next;
    	if (q->head == NULL) {
        	q->tail = NULL;         // last node removed
    	}
    	free(oldHead);
    	pthread_mutex_unlock(&q->mutex);
   		return 1;
	}


	//case 2: the requested node is somewhere in the middle or at the end
	if(!q->head || !q->head->next) {
		//there was only 1 element and it wasn't the one we wanted
		pthread_mutex_unlock(&q->mutex);
		return 0;
	}

	//using fast/slow method for deleting arbitrarily from a list of len > 1
	MessageNode* fast = q->head->next;
	MessageNode* slow = q->head;

	//until we get to the end of the list
	while(!fast) {
		//this is the node we want
		if(strcmp(q->head->msg.filename, filename) == 0 && strcmp(q->head->msg.diskFilename, diskFilename) == 0) {
			*msg_out = fast->msg;
			slow->next = slow->next->next;
			free(fast);
			pthread_mutex_unlock(&q->mutex);
			return 1;
		}
		//not the node we want, keep walking
		else{
			slow = fast;
			fast = fast->next;
		}
	}
    //Release lock
    pthread_mutex_unlock(&q->mutex);

    return success;
}