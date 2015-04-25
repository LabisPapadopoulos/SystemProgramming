#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "cache.h"

typedef struct doubleLinkedListNode{
	time_t time;
	int telephone;
	char data[MAX_DATA];
	struct doubleLinkedListNode *next;
	struct doubleLinkedListNode *previous;
}dnode; /* Kathe komvos kai eggrafh */

typedef struct singleLinkedListNode{
	dnode *record;
	struct singleLinkedListNode *next;
}bucket;

/* hashtable: Pinakas apo deiktes se buckets */
bucket **hashtable;

dnode *first;
dnode *last;
FILE *fp;
int maxRecords, countRecord;

int hash_function( int telephone )
{
	return telephone % maxRecords;
}

int initialize( FILE * file, int max_records )
{
	/*Arxikopoihsh hashtable */
	if( (hashtable = (bucket **)malloc(max_records * sizeof(bucket*))) == NULL )
	{
		printf("Den borese na desmeutei o hashtable!\n");
		return -1;
	}

	first = NULL;
	last = NULL;
	fp = file;
	/* Megisto plithos eggrafwn pou borw na exw stin cache */
	maxRecords = max_records;
	countRecord = 0;

	return 0;
}

int search( int telephone, char * record )
{
	dnode *newnode, *temp;
	char buffer[MAX_DATA];
	int tel, slot, delete_slot;
	bucket *new_bucket, *buck, **bucket_traverse, *bucket_temp;

	slot = hash_function( telephone );

	for( buck = hashtable[slot]; buck != NULL; buck = buck->next )
	{
		if( buck->record->telephone == telephone )
		{
			strncpy(record, buck->record->data, MAX_DATA);
			/* O buck->record == komvos tis sundedemenhs listas den einai prwtos, opote prepei na bei prwtos */
			if( buck->record != first )
			{
				/* Vgazw ton komvo apo tin lista */
				/* O previous tou komvou tha uparxei panda giati den tha einai pote prwtos komvos */
				/* O epomenos tou proigoumenou tou komvou tha einai o epomenos tou komvou */
				buck->record->previous->next = buck->record->next;
				/* O prohgoumenos tou epomenou tou komvou tha deixnei ston prohgoumeno tou komvou */
				if(buck->record->next != NULL)
					buck->record->next->previous = buck->record->previous;
				/* An o buck->record == komvos tis sundedemenhs listas htan teleutaios komvos, 
				vazoume ton deikth last na deixnei ston	akribws prohgoumeno */
				if(buck->record == last)
					last = last->previous;
				/* Topothetish tou komvou stin arxh tis listas */
				buck->record->next = first; /* 1) o epomenos tou komvou deixnei ston palio prwto */
				buck->record->previous = NULL; /* 2) den exei prohgoumeno o komvos */
				first->previous = buck->record; /* 3) O palios prwtos, pou sigoura uparxei,
								tha exei gia prohgoumeno ton komvo*/
				first = buck->record; /* 4) o prwtos komvos einai pleon 
							o komvos tis sundedemenhs listas (buck->record) */
				
			}
			printf("To vrika sth CACHE!\n");				
			return 1;
		}
	}

	/* Den to vrike stin dipla sundedemeni lista stin CACHE */
	/* Psaxnei sto arxeio */
	/* Arxikopoihsh tou fp 0 theseis apo tin arxi */
	if(fseek(fp, 0L, SEEK_SET) != 0)
	{
		printf("Den boresa na paw stin arxi tou arxeiou!\n");
		return -1;
	}

	while( !feof(fp) )
	{
		if( fgets( buffer, MAX_DATA, fp ) == NULL ) 		
		{
			if( feof(fp) )
				break;
				
			printf("Den boresa na diavasw apo to arxeio!\n");
			return -1;
		}
		/* Diavazei apo string enan akeraio -> thlefwno, string -> tin upoloiph eggrafh */
		/* %[^\n] -> diavase ta panda (monokomata) ekos apo tin allagh grammhs \n*/
		if(sscanf(buffer, "%d;%[^\n]", &tel, record ) != 2) /* to tilefwno kai to upoloipo string*/
		{
			printf("Den boresa na xwrisw tin eggrafh se thlefwno kai se upoloipo string\n");
			return -1;
		}
		if ( tel == telephone )
		{
			/* Afou vrethike to thlefwno, sigoura einai h pio prosfath eggrafh
			opote thn vazoume stin arxh */
			if( (newnode = (dnode *) malloc(sizeof(dnode)) ) == NULL )
			{
				printf("Den borese na dhmeiourgithei neos komvos!\n");
				return -1;
			}
			countRecord++;
			/* Apothikeush xronikis stigmhs pou vrethike h eggrafh */
			newnode->time = time(NULL);
			newnode->telephone = tel;
			strncpy(newnode->data, record, MAX_DATA);

			newnode->next = first; /* 1) o epomenos tou neou komvou deixnei ston palio prwto */
			newnode->previous = NULL; /* 2) den exei prohgoumeno o neos komvos */
			if( first != NULL ) /* 3) An uphrxe palios prwtos, tha exei gia prohgoumeno 
						ton neo komvo*/
				first->previous = newnode;
			first = newnode; /* 4) o prwtos komvos einai pleon o neos komvos */
			if(last == NULL) /* 5) otan uparxei mono mia eggrafh o deikths last deixnei  
						ston prwto komvo*/
				last = newnode;
			
			/* Eisagwgh ths kainourias egrafhs sto hashtable */
			slot = hash_function( telephone );

			if((new_bucket = (bucket*)malloc( sizeof(bucket) )) == NULL)
			{
				printf("Den boresa na desmeusw bucket sto slot tou hash table!\n");
				return -1;
			}

			/* Topothetish tis eggrafhs stin neo komvo tis apla sundedemenhs listas */
			new_bucket->record = newnode;
			/* topothetish tou neou komvou stin arxh tis apla sundedemenhs listas */
			new_bucket->next = hashtable[slot];
			hashtable[slot] = new_bucket;

			if( countRecord > maxRecords ) /* Exoume polles eggrafes, opote petame tin teleutaia */
			{
				/* Vriskw to slot sto opoio tha anhkei to bucket pros diagrafh == 
								to pio palio pros anazhthsh thlefwno  */
				delete_slot = hash_function( last->telephone );

				/* Xrhsh diplou deikth gia na allaxw ton idio ton deikth xwris na krataw ton
				prohgoumeno komvo */
				/* Me diplo deikth kanw perasma s' olh tin bucket list kai psaxnw na vrw
				to bucket tis teleutaias egrafhs gia na to afairesw apo tin lista */
				for( bucket_traverse = &(hashtable[delete_slot]); 
						(*bucket_traverse) != NULL; bucket_traverse = &( (*bucket_traverse)->next) )
				{
					/* An tairiazoun ta thlefwna tou teleutaiou komvou tis dipla sundedemenhs
					listas me to thlefwno ths eggrafhs tou bucket */
					if( (*bucket_traverse)->record->telephone == last->telephone)
					{
						/* Krataw se proswrino deikth ton komvo gia na ton kanw free */
						bucket_temp = *bucket_traverse;
						/* Allazw ton deikth na deixnei ston epomeno komvo apo auton
						pou tha diagrafei */
						*bucket_traverse = (*bucket_traverse)->next;
						free(bucket_temp);
						break;
					}
				}

				last->previous->next = NULL;
				temp = last;
				last = temp->previous;
				free(temp);
			}
			
			printf("To vrika sto arxeio!\n");
			
			return 1;
		}
	}
	return 0;
}

void finalize( void )
{
	dnode *ptr, *temp;
	int i;
	bucket *bucket_ptr, *bucket_temp;

	/* Diagrafh ths diplhs sundedemenhs listas */
	for( ptr = first; ptr != NULL; ptr = temp)
	{
		temp = ptr->next;
		free(ptr);
	}

	for( i = 0; i < maxRecords; i++ )
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
