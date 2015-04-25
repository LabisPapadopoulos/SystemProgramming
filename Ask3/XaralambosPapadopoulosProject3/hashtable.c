#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashtable.h"

typedef struct singleLinkedListNode{
	char *url;
	struct singleLinkedListNode *next;
}bucket;

/* hashtable: Pinakas apo deiktes se buckets */
bucket **hashtable;
int hashTableSize;

int hash_function( char *url )
{
	int sum = 0;

	/* Prostithetai o ascii kwdikos tou kathe xaraktira
	sto sum gia na ginei % me to megethos tou hashtable */
	while ( *url != '\0' )
		sum += *(url++);

	return sum % hashTableSize;
}

int initializeHashtable( int size )
{
	/*Arxikopoihsh hashtable me olous tous deiktes se bucket pou periexei NULL */
	if( (hashtable = (bucket **)calloc(size , sizeof(bucket*))) == NULL )
	{
		printf("Den borese na desmeutei o hashtable!\n");
		return -1;
	}

	/* Megethos hashtable */
	hashTableSize = size;

	return 0;
}

int addHashtable( char *url )
{
	int slot;
	bucket *new_bucket, *buck;

	slot = hash_function( url );

	for( buck = hashtable[slot]; buck != NULL; buck = buck->next )
	{
		/* Vrethike to url, opote den xana benei */
		if( strcmp(buck->url, url) == 0 )
			return 1;
	}

	/* Den vrike to url sto hashtable, opote to prosthetei */

	if((new_bucket = (bucket*)malloc( sizeof(bucket) )) == NULL)
	{
		printf("Den boresa na desmeusw bucket sto slot tou hash table!\n");
		return -1;
	}
	/* Topothetish tou url stin neo komvo tis apla sundedemenhs listas */
	new_bucket->url = url;
	/* topothetish tou neou komvou stin arxh tis apla sundedemenhs listas */
	new_bucket->next = hashtable[slot];
	hashtable[slot] = new_bucket;

	return 0;
}

void cleanupHashtable( void )
{
	int i;
	bucket *bucket_ptr, *bucket_temp;

	for( i = 0; i < hashTableSize; i++ )
	{
		/* Diagrafh twn buckets tou hashtable */
		for( bucket_ptr = hashtable[i]; bucket_ptr != NULL; bucket_ptr = bucket_temp )
		{
			bucket_temp = bucket_ptr->next;
			free(bucket_ptr);
		}
	}
	/* Apodesmeush tou hashtable */
	free(hashtable);
}
