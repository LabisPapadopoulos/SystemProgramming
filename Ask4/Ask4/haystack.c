#include <stdio.h>
#include <stdlib.h>
#include "semaphore.h"

#define MAGIC_NUMBER	169
#define BUFFER_SIZE	1024	

/* Domh euretiriou - lista */
typedef struct indexNode{
	int id;
	long offset;
	struct indexNode *next;
}indexNode;

/* Domh pliroforiwn gia to needle arxeio */
typedef struct{
	int id;
	int status;
	int length;
}needleHeader;

indexNode *index = NULL;
FILE *haystack = NULL;
int nextID = 0;

int compact( FILE *, FILE * );
int copyFile( FILE *, FILE * );

int initialize( char *fileName )
{
	int magicNumber = MAGIC_NUMBER, needles;
	FILE *tmp;

	/* Elegxos an uparxei to arxeio hdh */
	if ( (haystack = fopen( fileName, "rb")) == NULL )
	{ /* Den uparxei to arxeio haystack */
	
				/* a+: append sto telos, b: binary */
		if( (haystack = fopen( fileName, "a+b" )) == NULL )
			return -1;

		/* Dhmiourgia SuperBlock sto arxeio */
		if( fwrite( &magicNumber, sizeof(int), 1, haystack ) != 1 )
			return -1;
	}

	if(fclose( haystack ) == EOF)
		return -1;

	haystack = NULL;

	if( (haystack = fopen( fileName, "rb" )) == NULL )
		return -1;

	/* Elegxos an einai haystack file, me to SuperBlock */
	if( fread( &magicNumber, sizeof(int), 1, haystack ) != 1 ) 
	{
		if (ferror(haystack))
			return -1;
		else
			return -2;
	}

	if ( magicNumber != MAGIC_NUMBER )
		return -2; /* Auto pou anoixe den einai haystack file */

	/* Anoigma enos proswrinou arxeiou gia to compact */
	if( (tmp = tmpfile()) == NULL)
		return -1;
	
	if( (needles = compact( haystack, tmp )) == -1 )
		return -1;

	if(fclose( haystack ) == EOF)
		return -1;

	haystack = NULL;

	if( remove( fileName ) == -1 )
		return -1;
				/* w+: grafei diavazei opoudipote, b: binary */
	if( (haystack = fopen( fileName, "w+b" )) == NULL )
		return -1;

	if( copyFile( tmp, haystack ) == -1 )
		return -1;

	if( fclose( tmp ) == EOF )
		return -1;

	if ( initializeSemaphore() == -1 )
		return -1;

	return needles;
}

int finalize( void )
{
	indexNode *temp = NULL;

	if (haystack != NULL)
	{
		if(fclose(haystack) == EOF)
			return -1;
	}

	/* katharismos listas */
	while( index != NULL )
	{
		temp = index;
		index = index->next;
		free( temp );
	}

	if( finalizeSemaphore() == -1 )
		return -1;

	return 0;
}

int addNeedle( int length, void *binaryData )
{
	needleHeader header;
	indexNode *newIndexNode = NULL;
	long offset = 0;

	/* katevasma semaphorou */
	if ( down() == -1 )
		return -1;

	header.id = nextID;
	header.status = 1;
	header.length = length;

	/* Topothethsh tou fp sto telos */		
	if( fseek( haystack, 0, SEEK_END ) == -1 )
	{
		up(); /* se periptwsh sfalmatos, apanafora
			tou shmaforou stin arxikh tou katastash */
		return -1;
	}

	/* Epistrefei pou vriksetai mesa sto arxeio
	prin xekinisei na grafei to needle */
	if( (offset = ftell( haystack )) == -1 )
	{
		up();
		return -1;
	}

	/* Arxika pothikeuetai h domh plhroforiwn gia to needle */
	if( fwrite( &header, sizeof(header), 1, haystack) != 1 )
	{
		up();
		return -1;
	}

	/* Grafei sto haystack length bytes */
	if( fwrite( binaryData, 1, length, haystack ) != length )
	{
		up();
		return -1;
	}

	/* Adeiazei ton buffer gia na perastoun ola ta write sto haystack */
	if( fflush( haystack ) == EOF ) /* Gia exasfalish oti ta dedomena exoun
					graftei sto arxeio. */
	{
		up();
		return -1;
	}

	/* --- Topothetish sto index --- */
	if( (newIndexNode = (indexNode *) malloc (sizeof(indexNode))) == NULL )
	{
		up();
		return -1;
	}

	newIndexNode->id = nextID++;
	newIndexNode->offset = offset;
	newIndexNode->next = index;
	index = newIndexNode;

	/* anevasma shmaforou */
	if ( up() == -1 )
		return -1;

	return newIndexNode->id;
}

int searchNeedle( int id, int *length, void **binaryData )
{
	indexNode *traverse = NULL;
	needleHeader header;

	*length = 0;
	*binaryData = NULL;

	if( down() == -1 )
		return -1;

	for( traverse = index; traverse != NULL; traverse = traverse->next )
	{
		if( traverse->id == id )
		{
			/* Phgainei offset bytes apo tin arxh (SEEK_SET) tou haystack */
			if( fseek( haystack, traverse->offset, SEEK_SET ) == -1 )
			{
				up();
				return -1;
			}

			if( fread( &header, sizeof(header), 1, haystack ) != 1 )
			{
				up();
				return -1;
			}

			*length = header.length;

			if( (*binaryData = (void *) malloc ( *length )) == NULL )
			{
				*length = 0;
				up();
				return -1;
			}

					/* length antikeimena tou 1os byte */
			if( fread( *binaryData, 1, *length, haystack ) != *length )
			{
				*length = 0;
				free(*binaryData);
				*binaryData = NULL;
				up();
				return -1;			
			}
			
			if( up() == -1 )
				return -1;

			return 1;
		}
	}

	if( up() == -1 )
		return -1;

	return 0; /* Den vrike tipota */
}

int removeNeedle( int id )
{
	indexNode **traverse, *temp;
	needleHeader header;

	if( down() == -1 )
		return -1;

	for( traverse = &index; (*traverse) != NULL; traverse = &((*traverse)->next) )
	{
		if( (*traverse)->id == id )
		{
			/* Phgainei offset bytes apo tin arxh (SEEK_SET) tou haystack */
			if( fseek( haystack, (*traverse)->offset, SEEK_SET ) == -1 )
			{
				up();
				return -1;
			}

			if( fread( &header, sizeof(header), 1, haystack) != 1 )
			{
				up();
				return -1;
			}

			header.status = 0;

			/* xana gurizei pisw sto offset pou eixe diavasei gia na 
			xana grapsei thn idia domh panw sthn allh */
			if( fseek( haystack, (*traverse)->offset, SEEK_SET ) == -1 )
			{
				up();
				return -1;
			}

			if( fwrite( &header, sizeof(header), 1, haystack) != 1 )
			{
				up();
				return -1;
			}
		
			if( fflush( haystack ) == EOF )
			{
				up();
				return -1;
			}

			temp = *traverse;
			(*traverse) = (*traverse)->next;
			free( temp );

			if( up() == -1 )
				return -1;


			return 1;
		}
	}

	if( up() == -1 )
		return -1;

	return 0; /* Den vrike tipota */
}

int compact( FILE *input, FILE *output )
{
	needleHeader header;
	indexNode *newIndexNode;
	int magicNumber, needles = 0;
	void *buffer;

	/* Topothetish tou fp stin arxh (SEEK_SET) tou arxeiou */
	if( fseek( input, 0, SEEK_SET ) == -1 )
		return -1;

	if( fseek( output, 0, SEEK_SET ) == -1 )
		return -1;

	if( fread( &magicNumber, sizeof(int), 1, input ) != 1 )
		return -1;

	if( fwrite( &magicNumber, sizeof(int), 1, output ) != 1 )
		return -1;

	/* prospathei na diavasei oso uparxoun headers */
	while( fread( &header, sizeof(header), 1, input) )
	{
		if( header.status ) /* An to header status den einai 0 */
		{
			if( fwrite( &header, sizeof(header), 1, output ) != 1 )
				return -1;

			if( (buffer = malloc( header.length )) == NULL )
				return -1;

			if( fread( buffer, 1, header.length, input ) != header.length )
			{
				free( buffer );
				return -1;
			}

			if( fwrite( buffer, 1, header.length, output ) != header.length )
			{
				free( buffer );
				return -1;
			}

			free( buffer );

			/* xekinaei na metraei apo to epomeno id */
			nextID = header.id + 1;

			/* Dhmiourgia tis listas tou euretiriou sumfwna me to kainourio arxeio */
			if( (newIndexNode = (indexNode *) malloc (sizeof(indexNode))) == NULL )
				return -1;

			newIndexNode->id = header.id;

			/* euresh neou offset gia to output */
			if( (newIndexNode->offset = ftell( output )) == -1)
				return -1;

			/*Gia na xekinaei to offset stin arxh tou needle */
			newIndexNode->offset -= header.length + sizeof(needleHeader);
			newIndexNode->next = index;
			index = newIndexNode;

			
			needles++;
		}
		/* header status einai 0 */
		/* Metatopish tou fp apo to arxeio input kata length bytes pio katw */
		else if( fseek( input, header.length, SEEK_CUR ) == -1 )
			return -1;
	}

	/* An stamatise to while epeidh vrike sfalma sto arxeio */
	if( ferror(input) )
		return -1;

	if (fflush(output) == EOF)
		return -1;

	return needles;
}

int copyFile( FILE *input, FILE *output )
{
	char buffer[BUFFER_SIZE];
	int bytesRead;

	if( fseek( input, 0, SEEK_SET ) == -1 )
		return -1;

	if( fseek( output, 0, SEEK_SET ) == -1 )
		return -1;

	/* BUFFER_SIZE * sizeof(char) kommatia tou enos byte */
	while( (bytesRead = fread( buffer, 1, BUFFER_SIZE * sizeof(char), input)) > 0 )
		if( fwrite( buffer, 1, bytesRead, output ) != bytesRead )
			return -1;

	if( ferror(input) )
		return -1;

	if ( fflush(output) == EOF )
		return -1;

	return 0;
}

