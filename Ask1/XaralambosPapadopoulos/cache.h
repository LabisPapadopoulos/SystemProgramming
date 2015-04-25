#ifndef CACHE_H
#define CACHE_H

#define MAX_DATA 128

/* Arxikopoihsh arxeiou gia anazitish */
int initialize( FILE *, int );

int search( int , char * );

void finalize( void );

#endif

