#ifndef HAYSTACK_H
#define HAYSTACK_H

int initialize( char * );

int finalize(void);

int addNeedle( int , void * );

int searchNeedle( int , int *, void ** );

int removeNeedle( int );

#endif
