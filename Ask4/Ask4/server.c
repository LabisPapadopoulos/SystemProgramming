#include <stdio.h>
#include <stdlib.h> /* exit() */
#include <string.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h> /* close, read, write */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "http.h"
#include "haystack.h"

#define PENDING_CONNECTIONS 	5

typedef struct threadNode {
	pthread_t thread;
	struct threadNode *next;
}threadNode;

threadNode *threads = NULL;

void *serve( void * );
void logTime( void );
void terminate( int );

/* Global to serverSocketDesc gia na to
vlepei kai h terminate */
int serverSocketDesc;

int main(int argc, char **argv)
{	
	int clientSocketDesc;

	/* Plirofories me tin dieuthinsh tou server
	pou tha akouei. */
	struct sockaddr_in server, client; /* in: internet dieuthinsh (IPv4) */
	socklen_t clientLength;

	threadNode *newThread = NULL;
	char *fileName = NULL;
	int needles, i, port = -1;

	for( i = 0; i < argc; i++ )
	{
						/* an uparxei to orisma */
		if( ( strcmp("-p", argv[i]) == 0 ) && (i + 1 < argc ) )
			port = atoi( argv[i + 1] );

		
		else if( ( strcmp("-f", argv[i]) == 0 ) && (i + 1 < argc ) )
			fileName = argv[i + 1];
	}

	if( (fileName == NULL) || (port == -1) )
	{
		fprintf(stderr, "usage: %s -p <port> -f <filename>\n", argv[0]);
		return -1;
	}

	logTime();
	fprintf(stderr, "Server is starting.\n" );

	if( signal( SIGINT , terminate ) == SIG_ERR )
	{
		logTime();
		perror( "Error setting termination handler" );
		terminate(0);
	}

	logTime();
	fprintf(stderr, "Loaded haystack file %s\n", fileName);

	logTime();
	fprintf(stderr, "Creating in-memory datastructures...\n");

	if( (needles = initialize( fileName )) == -1 )
	{
		logTime();
		perror("Error loading haystack file");
		terminate(0);
	}
	else if( needles == -2 ) /* Auto pou anoixe den einai haystack file */
	{
		logTime();
		fprintf(stderr, "Invalid haystack file\n");
		terminate(0);
	}
	
	logTime();
	fprintf(stderr, "%d files-needles found in the haystack file\n", needles);
	
	/* Dhmiourgreia socket */
			    /* domain: IPv4 type: TCP, protocol: 0  */
	if ( (serverSocketDesc = socket( AF_INET, SOCK_STREAM, 0 )) == -1 )
	{
		logTime();
		perror( "Error creating server socket" );
		terminate(0);
	}

	/* internet (IPv4) dieuthinsh */
	server.sin_family = AF_INET;
	/* Epitrepei na dexetai sundeseis apo opoiadhpote dieuthinsh (INADDR_ANY) */
	/* htonl: apo tin morfh pou exei ta dedomena to mixanima metatrepontai se
	endianes tou diktiou */
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	/* Kathorizei tin porta stin opoia tha akouei o server */
	server.sin_port = htons( port );

	/* Desmeuei to socket na akouei se mia porta */
			  /* H dieuthinsh tou server */
	if( bind( serverSocketDesc, (struct sockaddr *) &server , sizeof(server) ) == -1 )
	{
		logTime();
		fprintf( stderr, "Error binding server socket at port %d: ", port );
		perror(NULL);
		terminate(0);
	}

	/* To socket anoigei kai akouei tis eiserxomenes sundeseis */
	if( listen( serverSocketDesc, PENDING_CONNECTIONS ) == -1 )
	{
		logTime();
		perror( "Error listening to server socket" );
		terminate(0);
	}

	logTime();
	fprintf( stderr, "Server is ready for new clients!\n" );

	while(1)
	{
		clientLength = sizeof(struct sockaddr_in);
		/* Perimenei mexri na erthei kapoios sto socket kai gemizei me tin
		dieuthinsh tou client kai to megethos tou. Molis petuxei dhmiourgei
		neo socket se allh porta gia epikoinwnisei me ton client */
		if( (clientSocketDesc = accept( serverSocketDesc, ( struct sockaddr *)&client, &clientLength )) == -1 )
		{
			logTime();
			perror( "Error accepting client" );
			terminate(0);
		}


		if( (newThread = (threadNode *) malloc ( sizeof(threadNode) )) == NULL )
		{
			logTime();
			perror( "Error accepting client" );
			terminate(0);
		}

		/* Dhmiourgeia Thread meta tin accept */
		/* thread: pou apothikeuetai to thread, NULL: exei default attributes gia to nima
		serve: h sunartish pou kalei kai arg: ta orismata tis sunartishs */
		if ( pthread_create( &(newThread->thread) , NULL, serve, &clientSocketDesc ) != 0 )
		{
			logTime();
			perror( "Error creating thread" );
			terminate(0);
		}

		newThread->next = threads;
		threads = newThread;
	}
}

/* Sunartish pou tha exuphretei ton kathe ena client */
void *serve( void *clientSocketDesc )
{
	int id, contentLength;
	char *host;
	void *binaryData;

	switch (parseRequest( *((int *)clientSocketDesc), &id, &host, &contentLength, &binaryData ))
	{
	case UPLOAD:
		logTime();
		fprintf(stderr, "Client [%s] requested a file upload with %d content length\n", host, contentLength);

		if ( (id = addNeedle( contentLength, binaryData )) == -1)
		{ /* sfalma */
			logTime();										/* Perigrafh tou teleutaiou sfalmatos */
			fprintf(stderr, "Client [%s] file upload failed due to some internal server error: %s\n", host, strerror(errno));

			if ( sendResponse( *((int *) clientSocketDesc), SERVER_ERROR, 0, 0, NULL) == -1 )
			{
				logTime();
				fprintf(stderr, "Internal server error while sending response: %s\n", strerror(errno));
			}
		}
		else
		{
			logTime();
			fprintf(stderr, "Client [%s] file upload finished successfully\n", host);

			if ( sendResponse( *((int *) clientSocketDesc), UPLOAD, id, 0, NULL) == -1 )
			{
				logTime();
				fprintf(stderr, "Internal server error while sending response: %s\n", strerror(errno));
			}
		}
		if (host != NULL)
			free( host );
		if (binaryData != NULL)
			free( binaryData );
		break;
	case DOWNLOAD: /* GET */
		logTime(); 
		fprintf(stderr, "Client [%s] requested downloading file with id : %d\n", host, id);

		switch( searchNeedle( id, &contentLength, &binaryData ) ){
		case 0: /* Den vrike tipota */
			logTime();
			fprintf(stderr, "Client [%s] File with id : %d was not found\n", host, id);

			/* Apostolh apantishs ston client mesw tou socket */
			if ( sendResponse( *((int *) clientSocketDesc), NOT_FOUND, 0, 0, NULL) == -1 )
			{
				logTime();
				fprintf(stderr, "Internal server error while sending response: %s\n", strerror(errno));
			}

			break;
		case -1: /* Eswteriko sfalma */
			logTime();
			fprintf(stderr, "Client [%s] File with id : %d could not be downloaded due to some internal server error: %s\n",host, id, strerror(errno));

			if ( sendResponse( *((int *) clientSocketDesc), SERVER_ERROR, 0, 0, NULL) == -1 )
			{
				logTime();
				fprintf(stderr, "Internal server error while sending response: %s\n", strerror(errno));
			}

			break;
		case 1:/* Download ok! */
			logTime();
			fprintf(stderr, "Client [%s] successfully finished downloading file with id : %d. Total bytes tranfered : %d.\n", host, id, contentLength);

			if ( sendResponse( *((int *) clientSocketDesc), DOWNLOAD, 0, contentLength, binaryData) == -1 )
			{
				logTime();
				fprintf(stderr, "Internal server error while sending response: %s\n", strerror(errno));
			}

			break;
		}
		if (host != NULL)
			free( host );
		if (binaryData != NULL)
			free( binaryData );
		break;
	case DELETE:
		logTime(); 
		fprintf(stderr, "Client [%s] requested deletion of file with id : %d\n", host, id );

		switch( removeNeedle(id) ){
		case 0: /* Den vrike tipota */
			logTime();
			fprintf(stderr, "Client [%s] File with id : %d was not found\n", host, id );

			if ( sendResponse( *((int *) clientSocketDesc), NOT_FOUND, 0, 0, NULL) == -1 )
			{
				logTime();
				fprintf(stderr, "Internal server error while sending response: %s\n", strerror(errno));
			}

			break;
		case -1: /* Eswteriko sfalma */
			logTime();
			fprintf(stderr, "Client [%s] File with id : %d could not be deleted due to some internal server error : %s\n", host, id, strerror(errno));

			if ( sendResponse( *((int *) clientSocketDesc), SERVER_ERROR, 0, 0, NULL) == -1 )
			{
				logTime();
				fprintf(stderr, "Internal server error while sending response: %s\n", strerror(errno));
			}

			break;
		case 1: /* Delete ok! */
			logTime();
			fprintf(stderr, "Client [%s] File with id : %d was successfully deleted\n", host, id );

			if ( sendResponse( *((int *) clientSocketDesc), DELETE, id, 0, NULL) == -1 )
			{
				logTime();
				fprintf(stderr, "Internal server error while sending response: %s\n", strerror(errno));
			}

			break;
		}
		break;
	case BAD_REQUEST:
		logTime();
		fprintf( stderr, "Client [%s] request errror.\n", host );

							/* einai bad request, opote den tou grafei kati sugkekrimeno */
		if ( sendResponse( *((int *) clientSocketDesc), BAD_REQUEST, 0, 0, NULL) == -1 )
		{
			logTime();
			fprintf(stderr, "Internal server error while sending response: %s\n", strerror(errno));
		}

		if (host != NULL)
			free( host );
		if (binaryData != NULL)
			free( binaryData );
		break;
	case SERVER_ERROR:
		logTime();
		fprintf(stderr, "Internal server error while parsing request : %s\n", strerror(errno));

		if ( sendResponse( *((int *) clientSocketDesc), SERVER_ERROR, 0, 0, NULL) == -1 )
		{
			logTime();
			fprintf(stderr, "Internal server error while sending response: %s\n", strerror(errno));
		}

		if (host != NULL)
			free( host );
		if (binaryData != NULL)
			free( binaryData );
		break;
	}

	if(close(*((int *) clientSocketDesc)) == -1 )
	{
		logTime();
		fprintf(stderr, "Internal server error while closing socket: %s\n", strerror(errno));
		return NULL;
	}

	return NULL;
}

void logTime( void )
{
	time_t timestamp;
	char *string;

	/* H time epistrefei timestamp kai h ctime to metatrepei se string kanonikhs hmerominias*/
	if ((time(&timestamp) != -1) && ((string = ctime(&timestamp)) != NULL))
	{
		string[strlen(string) - 1] = '\0';
		fprintf(stderr, "[%s] ", string);
	}
}

void terminate( int signum )
{
	threadNode *temp = NULL;
	void *retval;

	fprintf(stderr, "\n");
	logTime();
	fprintf(stderr, "Server is shutting down . Server will not accept any other clients\n");

	/* Gia na min dexetai nees ethseis kleinei to socket */
	if( close(serverSocketDesc) == -1 )
	{
		logTime();
		perror( "Error closing server socket" );
	}
	logTime();
	fprintf(stderr, "Waiting for running clients to be served ...\n");

	/* katharismos listas me threads */
	while( threads != NULL )
	{
		/* join ta threads tis listas */
		/* O server (to arxiko nhma) perimenei ola ta paidia
		na termatisoun kai etsi tha exoun ekplirwthei oles oi
		trexouses aithseis */
		if( pthread_join( threads->thread, &retval ) != 0 )
		{
			logTime();
			perror("Error joining thread");
		}

		temp = threads -> next;
		free(threads);
		threads = temp;
	}

	logTime();
	fprintf(stderr, "Cleaning up ...\n");
	finalize();
	logTime();
	fprintf(stderr, "Bye\n");
	exit(0);
}

