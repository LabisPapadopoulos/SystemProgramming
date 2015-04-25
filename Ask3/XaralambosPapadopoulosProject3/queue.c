#include <stdio.h>
#include <stdlib.h>

/* Domi apothikeushs twn URLs se oura */
typedef struct QueueNode{
	char *url;
	struct QueueNode * next;
}QueueNode;

QueueNode * first = NULL;
QueueNode * last = NULL;

int addQueue(char *url) 
{
	QueueNode *newQueueNode;

	if((newQueueNode = (QueueNode *) malloc(sizeof(QueueNode))) == NULL)
	{
		perror("Den boresa na desmeusw ton neo komvo tis ouras");
		
		return -1;
	}

	newQueueNode->url = url;
	newQueueNode->next = NULL;

	if(last != NULL ) /* an exei toulaxiston ena komvo h oura */
		last->next = newQueueNode;
	else
	/* h oura den exei kanena komvo, opote autos o komvos
	einai kai prwtos kai teleytaios */
		first = newQueueNode;

	/* proxwraei o deikths last sto telos */
	last = newQueueNode;

	return 0;	
}

char* removeQueue( void )
{
	QueueNode *temp;
	char *url;

	if(first == NULL)
		return NULL;

	temp = first;
	first = first->next;
	/* an h oura eixe mono ena stoixeio, twra 
	pou vgike einai teleiws adeio (last = NULL) */
	if (first == NULL)
		last = NULL;

	/* epistrefei to url tou 1ou komvou
	pou deixnei pros to paron o temp */
	url = temp->url;
	free(temp);

	return url;
}

