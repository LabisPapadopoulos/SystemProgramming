#ifndef HTTP_H
#define HTTP_H

#define UPLOAD			1
#define DOWNLOAD 		2
#define DELETE			3
#define BAD_REQUEST		-2
#define NOT_FOUND		-3
#define SERVER_ERROR		-1

int parseRequest( int, int *, char **, int *, void ** );
int sendResponse( int , int , int , int , void * );

#endif
