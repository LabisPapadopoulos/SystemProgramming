#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "http.h"

#define BUFFER_SIZE 		1024
#define MAX_RESPONSE_BODY	64
#define MAX_RESPONSE		1024	

#define UPLOAD_BODY		"File successfully uploaded with id : %d"
#define UPLOAD_MESSAGE		"HTTP/1.0 200 OK\r\nServer: haystack_server v1.0\r\nConnection: close\r\nContent-Type: text\r\nContent-Length: %d\r\n\r\n%s"

#define DOWNLOAD_MESSAGE	"HTTP/1.0 200 OK\r\nServer: haystack_server v1.0\r\nConnection: close\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\n\r\n"

#define DELETE_BODY		"File with id : %d was successfully deleted"
#define DELETE_MESSAGE		"HTTP/1.0 200 OK\r\nServer: haystack_server v1.0\r\nConnection: close\r\nContent-Type: text\r\nContent-Length: %d\r\n\r\n%s"

#define BAD_REQUEST_MESSAGE 	"HTTP/1.0 400 Bad Request\r\nServer: haystack_server v1.0\r\nConnection: close\r\n\r\n"
#define NOT_FOUND_MESSAGE 	"HTTP/1.0 404 Not Found\r\nServer: haystack_server v1.0\r\nConnection: close\r\n\r\n"
#define SERVER_ERROR_MESSAGE 	"HTTP/1.0 500 Internal Server Error\r\nServer: haystack_server v1.0\r\nConnection: close\r\n\r\n"

int parseRequestLine( char *, char *, int *);
int parseHeaderLine( char *, char *, char **, char ** );

int parseRequest( int clientSocketDesc, int *id, char **host, int *contentLength, void **binaryData )
{
	int bytesRead, fullRead, charsInBuffer, requestType, headerFound;
	char buffer[BUFFER_SIZE], *end, *headerName, *headerValue;

	*id = -1;
	*contentLength = 0;
	*host = NULL;
	*binaryData = NULL;

	if( (bytesRead = read( clientSocketDesc, buffer, (BUFFER_SIZE - 1) * sizeof(char) )) == -1 )
		return SERVER_ERROR;

	/* Diavase akrivws osa zhththhkan, epomenws mporei na uparxoun kai alla */
	fullRead = (bytesRead == ((BUFFER_SIZE - 1) * sizeof(char))) ?  1 : 0 ;
	charsInBuffer = bytesRead / sizeof(char);

	buffer[ charsInBuffer ] = '\0';

	if((end = strstr( buffer, "\r\n")) == NULL)
		return SERVER_ERROR; /* Den eftase to buffer gia parsarisma sto request line*/

	requestType = parseRequestLine( buffer, end, id );

	do
	{
		/* afairountai osoi xaraktires katanalwthikan apo to prohgoumeno parsing */
		charsInBuffer -= (end - buffer + strlen("\r\n"));
		/* aristerh olisthish auta pou den exoun diavastei akoma */
		memmove( buffer, end + strlen("\r\n"), charsInBuffer);

		/* An to prohgoumeno read eixe gemisei plhrws to buffer tote uparxei 
		pithanothta na uparxoun kai alla dedomena pou perimenoun na diavastoun*/
		if ( fullRead ) /* diavase osa tou zhtisa kai oxi ligotera */
		{
								/* xekinaei na diavazei apo ekei kai katw */
			if( (bytesRead = read( clientSocketDesc, buffer + charsInBuffer, (BUFFER_SIZE - 1 - charsInBuffer) * sizeof(char) )) == -1 )
				return SERVER_ERROR;
			/* Diavase akrivws osa zhththhkan, epomenws mporei na uparxoun kai alla
			An h teleutaia read pou ekane, gemise h oxi */
			fullRead = (bytesRead == (BUFFER_SIZE - 1 - charsInBuffer) * sizeof(char))  ?  1 : 0 ;
			charsInBuffer += (bytesRead / sizeof(char));
			buffer[ charsInBuffer ] = '\0';
		}

		if((end = strstr( buffer, "\r\n")) == NULL)
			return SERVER_ERROR; /* Den eftase to buffer gia parsarisma sto header line */

		headerFound = parseHeaderLine( buffer, end, &headerName, &headerValue );

		if( headerFound == SERVER_ERROR )
			return SERVER_ERROR;

		if( headerFound && ( strcmp( headerName, "Host" ) == 0 ) ) /* Vrethike o Host */
		{
			*host = headerValue;
			free( headerName );
		}
		else if( headerFound && ( strcmp( headerName, "Content-Length" ) == 0 ) ) /* Vrethike to Content-Length */
		{
			*contentLength = atoi(headerValue);
			free( headerName );
			free( headerValue );
		}
		else if (headerFound) /* Vrethike header pou den mas endiaferei */
		{
			free( headerName );
			free( headerValue );
		}

	}
	while( headerFound );

	/* afairountai osoi xaraktires katanalwthikan apo to prohgoumeno parsing */
	charsInBuffer -= (end - buffer + strlen("\r\n"));
	/* aristerh olisthish auta pou den exoun diavastei akoma */
	memmove( buffer, end + strlen("\r\n"), charsInBuffer);

	

	/* --- Binary Data --- */

	if( (*binaryData = malloc( *contentLength )) == NULL ) /* xuma data */
		return SERVER_ERROR;

	/* Gemise to binary data me ta dedomena pou uparxoun meta to teleutaio "\r\n" */
	if ( fullRead ) /* to buffer htan gemato, mporei na mhn xwrese ola ta binary data */
	{
		memcpy( *binaryData, buffer, charsInBuffer * sizeof (char) );
		/* diavazw ta ypoloipa binary data */
		bytesRead = read( clientSocketDesc, (*binaryData + charsInBuffer * sizeof(char)), *contentLength - (charsInBuffer * sizeof(char)) );

		if ( bytesRead == -1 ) /* sfalma sto diavasma */
		{
			free(*binaryData);
			*binaryData = NULL;
			*contentLength = 0;

			return SERVER_ERROR;
		}
		else if ( bytesRead != *contentLength - (charsInBuffer * sizeof(char)) ) /* diavase ligotera, provlima me auta pou esteile o client */
		{
			free(*binaryData);
			*binaryData = NULL;
			*contentLength = 0;

			return BAD_REQUEST;
		}
	}
	else /* to buffer den htane gemato, ola ta binary data einai hdh mesa se auto */
		memcpy( *binaryData, buffer, *contentLength);
	
	return requestType;
}

/* parsarei tin 1h grammh enos request */
int parseRequestLine( char *buffer, char *end, int *id )
{
	char *idString;

		/* to  HTTP/1.0 xwraei mesa stin 1h grammh  &&	Vrethike sto telos to HTTP/1.0 */
	if ( !(( (end - strlen(" HTTP/1.0") > buffer) && (strncmp( end - strlen(" HTTP/1.0"), " HTTP/1.0", strlen(" HTTP/1.0")) == 0 ) ) ||
			( (end - strlen(" HTTP/1.1") > buffer) && (strncmp( end - strlen(" HTTP/1.1"), " HTTP/1.1", strlen(" HTTP/1.1")) == 0 ) )) )
		return BAD_REQUEST; /* Den vrethike HTTP/1.0 h HTTP/1.1 sto request */

	if( (buffer + strlen("GET ") < end) && (strncmp( buffer ,"GET ", strlen("GET ")) == 0 ) ) /* Get aithma*/
	{
		if( (buffer + strlen("GET ") + strlen("/?id=") < end) && (strncmp( buffer + strlen("GET "), "/?id=", strlen("/?id=") ) == 0) ) /* Leitourgeia anaktishs eikonas */
		{
			/* Desmeush xwrou gia to id */
			if((idString = (char *) malloc ( (end - strlen(" HTTP/1.0") - buffer - strlen("GET ") - strlen("/?id=") + 1) * sizeof(char))) == NULL)
				return SERVER_ERROR;

			strncpy( idString, buffer + strlen("GET ") + strlen("/?id="), end - strlen(" HTTP/1.0") - buffer - strlen("GET ") - strlen("/?id=") );
			idString[ end - strlen(" HTTP/1.0") - buffer - strlen("GET ") - strlen("/?id=")] = '\0';

			*id = atoi( idString ); /* Apothikeutike to id */

			free( idString );

			return DOWNLOAD;
		}
		else if( (buffer + strlen("GET ") + strlen("/?d_id=") < end)  && strncmp( buffer + strlen("GET "), "/?d_id=", strlen("/?d_id=") ) == 0 ) /* Leitourgeia diagrafhs eikonas */
		{
			/* Desmeush xwrou gia to id */
			if((idString = (char *) malloc ( (end - strlen(" HTTP/1.0") - buffer - strlen("GET ") - strlen("/?d_id=") + 1) * sizeof(char))) == NULL)
				return SERVER_ERROR;

			strncpy( idString, buffer + strlen("GET ") + strlen("/?d_id="), end - strlen(" HTTP/1.0") - buffer - strlen("GET ") - strlen("/?d_id=") );
			idString[ end - strlen(" HTTP/1.0") - buffer - strlen("GET ") - strlen("/?d_id=")] = '\0';

			*id = atoi( idString ); /* Apothikeutike to id */

			free( idString );

			return DELETE;
		}
		else
			return BAD_REQUEST; /* Den vrike url swsto */
	}
	else if ( (buffer + strlen("POST ") < end) && (strncmp( buffer ,"POST ", strlen("POST ")) == 0 ) ) /* Post aithma - upload */
	{
		/* euresh tou "/" */
		if( (end - strlen(" HTTP/1.0") - buffer - strlen("POST ") == 1) && (buffer[strlen("POST ")] == '/') )
			return UPLOAD;

		return BAD_REQUEST; /* Den vrike '/' gia url */
	}
	else
		return BAD_REQUEST; /* H methodos den htan oute GET oute POST */

}

int parseHeaderLine( char *buffer, char *end, char **headerName, char **headerValue )
{
	char *header, *middle;

	*headerName = NULL;
	*headerValue = NULL;
	
	if( buffer == end ) /* Den uparxei tipota metaxu tous, dhl eftase sto telos twn headers (kenh grammh) */
		return 0;

	if( (header = (char *) malloc ( (end - buffer + 1) * sizeof(char) )) == NULL )
		return SERVER_ERROR;

	strncpy( header, buffer, end - buffer );
	header[ end - buffer ] = '\0';

	if( (middle = strstr(header, ": ")) == NULL )
	{
		free( header );
		return SERVER_ERROR;
	}

	/* euresh headerName */
	if( (*headerName = (char *)malloc( (middle - header + 1) * sizeof(char) )) == NULL )
	{
		free( header );
		return SERVER_ERROR;
	}

	strncpy( *headerName, header, middle - header );
	(*headerName)[middle - header] = '\0';

	/* euresh headerValue */
	if( (*headerValue = (char *)malloc( (strlen(header) - (middle - header) - strlen(": ") + 1) * sizeof(char) )) == NULL )
	{
		free( header );
		free( *headerName );
		*headerName = NULL;

		return SERVER_ERROR;
	}

	strncpy( *headerValue, middle + strlen(": "), strlen(header) - (middle - header) - strlen(": "));
	(*headerValue)[strlen(header) - (middle - header) - strlen(": ")] = '\0';

	free( header );

	return 1;
}

int sendResponse( int clientSocketDesc, int request, int id, int contentLength, void *binaryData )
{
	char body[MAX_RESPONSE_BODY], response[MAX_RESPONSE];

	switch( request )
	{
	case UPLOAD: /* 200 - ok! */
		if( (contentLength = sprintf(body, UPLOAD_BODY, id)) < 0) /* epistrefei xaraktires */
			return -1;
		contentLength *= sizeof(char); /* oi xaraktires metatrepontai se bytes */
		if( sprintf(response, UPLOAD_MESSAGE, contentLength, body) < 0)
			return -1;
		if(write( clientSocketDesc, response, strlen(response) * sizeof(char) ) !=  strlen(response) * sizeof(char))
			return -1;
		break;
	case DOWNLOAD: /* 200 - ok! */
		if( sprintf(response, DOWNLOAD_MESSAGE, contentLength) < 0)
			return -1;
		if(write( clientSocketDesc, response, strlen(response) * sizeof(char) ) !=  strlen(response) * sizeof(char))
			return -1;
		if(write( clientSocketDesc, binaryData, contentLength) !=  contentLength )
			return -1;
		break;
	case DELETE: /* 200 - ok! */
		if( (contentLength = sprintf(body, DELETE_BODY, id)) < 0) /* epistrefei xaraktires */
			return -1;
		contentLength *= sizeof(char); /* oi xaraktires metatrepontai se bytes */
		if( sprintf(response, DELETE_MESSAGE, contentLength, body) < 0)
			return -1;
		if(write( clientSocketDesc, response, strlen(response) * sizeof(char) ) !=  strlen(response) * sizeof(char))
			return -1;
		break;
	case BAD_REQUEST: /* 400 */
		if(write( clientSocketDesc, BAD_REQUEST_MESSAGE, strlen(BAD_REQUEST_MESSAGE) * sizeof(char) ) !=  strlen(BAD_REQUEST_MESSAGE) * sizeof(char))
			return -1;
		break;
	case NOT_FOUND: /* 404 */
		if(write( clientSocketDesc, NOT_FOUND_MESSAGE, strlen(NOT_FOUND_MESSAGE) * sizeof(char) ) !=  strlen(NOT_FOUND_MESSAGE) * sizeof(char))
			return -1;
		break;
	case SERVER_ERROR: /* 500 */
		if(write( clientSocketDesc, SERVER_ERROR_MESSAGE, strlen(SERVER_ERROR_MESSAGE) * sizeof(char) ) !=  strlen(SERVER_ERROR_MESSAGE) * sizeof(char))
			return -1;
		break;
	}

	return 0;
}

