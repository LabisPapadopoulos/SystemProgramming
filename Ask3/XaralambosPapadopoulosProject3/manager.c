#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

#include "child.h"
#include "queue.h"
#include "results.h"
#include "tree.h"
#include "hashtable.h"

#define MAX_LINE 1024

int kill(pid_t , int );

int loadUrl( char *url )
{
	if (strcmp(url, "") == 0)
		return 0;
	switch( addHashtable(url) )
	{
	case -1: /* yparxei provlima */
		fprintf(stderr,"Den boresa na valw to url sto hashtable\n");
		return -1;
	case 1: /* Vrethike to url sto hashtable */
		/* fprintf(stderr, "To url uparxei hdh sto hashtable\n"); */
		return 0;
	case 0: /* Den uphrxe sto hashtable kai molis bike kai
		prosthikh kai stin oura */
		return addQueue( url );
	}
	return 0;
}

/* kleinei ta pipes, kanei free to sxetiko pinaka, 
perimenei ta paidia na termatisoun kai meta kanei 
free ta pids */
void cleanUp( int maxChildren, pid_t *pids, int *pipeDesc, int *workQueue, int *relaxQueue )
{
	int i, status;

	if( pipeDesc != NULL )
	{
		for( i = 0; i < 4 * maxChildren; i++ )
		{
			if( pipeDesc[i] != -1)
			{
				/* kleisimo ola ta pipe ( pipe[0] kai pipe[1]) */
				if(close( pipeDesc[i]) == -1)
					perror("Apotuxeia sto kleisimo tis akrhs tou pipe");

				pipeDesc[i] = -1;
			}
		}
		free(pipeDesc);
		pipeDesc = NULL;
	}

	if(pids != NULL)
	{
		for( i = 0; i < maxChildren; i++ )
		{
			if(pids[i] != -1)
			{
				if( wait( &status ) == -1)
					perror("Provlima me tin wait tou paidiou stin cleanUp!");
				else if(status != 0 )
					fprintf(stderr, "To paidi %d termatise me sfalma!\n", pids[i] );
				else
					printf("To paidi %d termatise mia xara!\n", pids[i] );

				pids[i] = -1;
			}
			
		}
		free(pids);
		pids = NULL;
	}

	if( workQueue != NULL )
		free(workQueue);
	
	if( relaxQueue != NULL )
		free(relaxQueue);

}

int manage( int maxChildren, int maxUrls, FILE * outputFile )
{
	pid_t *pids;
	int i, j, *pipeDesc, linkSize, bytesRead, urlTypeSize, resultCounter, averageSize, maxSize;
	float maxTime, averageTime;
	char *link, *maxUrl = "";
	Results results;
	ListNode *newLink;
	/* Oura gia to poioi douleuoun sunolika */
	int *workQueue = NULL;
	/* Oura gia to poioi kathontai */
	int *relaxQueue = NULL;
	/* poioi douleuoun kai poioi kathontai pragmatika */
	int working, relaxing;

	pids = NULL;
	pipeDesc = NULL;

	/* Xeirismos shmatos tou patera, se periptwsh pou lavei shma apo kapoio paidi.
	Otan to lavei, to agnoei (SIG_IGN) kai sunexizei na perimenei simata apo alla paidia
	prin termatisei */
	if( signal( SIGRTMIN+2, SIG_IGN ) == SIG_ERR)
	{
		perror("Pateras: Den boresa na agnohsw to shma SIGRTMIN+2");
		return -1;
	}

	/* pinakas gia na apothikeuontai ta process id twn paidiwn */
	if( (pids = (pid_t *) malloc(maxChildren * sizeof(pid_t))) == NULL )
	{
		perror("Den boresa na desmeusw mnimh gia ta pids twn paidiwn");
		cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );
		return -1;
	}
	/* Arxikopoihsh me -1 gia na xerw pote einai axrisimopoihta */
	for( i = 0; i < maxChildren; i++ )
		pids[i] = -1;

	/* pinakas gia ta pipes epikoinwnias me ta paidia */
	/* Dimiourgeitai enas pinakas gia 2*maxChildren pipes, diladh gia 
	4 * maxChildren akres. Opote kathe paidi tha vlepei 2 akres grapsimatos
	kai 2 akres diavasmatos */
	if( (pipeDesc = (int *) malloc(4 * maxChildren * sizeof(int))) == NULL )
	{
		perror("Den boresa na desmeusw mnimh gia ta pipes twn paidiwn");
		cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );
		return -1;
	}

	for( i = 0; i < 4 * maxChildren; i++ )
		pipeDesc[i] = -1;
	
	if((workQueue = (int *) malloc ( maxChildren * sizeof(int) )) == NULL)
	{
		perror("Den boresa na desmeusw xoro gia tin oura twn paidiwn pou douleuoun");
		cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );
		return -1;
	}

	if((relaxQueue = (int *) malloc ( maxChildren * sizeof(int) )) == NULL)
	{
		perror("Den boresa na desmeusw xoro gia tin oura twn paidiwn pou kathontai");
		cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );
		return -1;
	}


	/* Dhmiourgeia paidiwn */
	for( i = 0; i < maxChildren; i++ )
	{
		/* Dhmiourgeia duo pipes ana tesseris theseis tou pinaka */
		if( pipe(pipeDesc + 4*i) == -1 )
		{
			perror("Den borese na dimiourgithei to pipe");
			cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );
			return -1;
		}
		/* Dhmiourgeia pipe gia tis epomenes duo theseis tou pinaka */
		if( pipe(pipeDesc + 4*i + 2) == -1 )
		{
			perror("Den borese na dimiourgithei to pipe");
			cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );
			return -1;
		}

		switch(pids[i] = fork())
		{
		case 0: /* paidi - child.c */

			/* kleinoun oles oi akres twn pipes pou den xrisimopoiountai 
			apo auto to paidi */
			for( j = 0; j < 4 * maxChildren; j++ )
			{
				if (pipeDesc[j] != -1) 
				{
					/* pipes pou xrisimopoiei to paidi */
					if( ( j != 4 * i) && ( j != 4 * i + 3 ) )
					{
						if(close(pipeDesc[j]) == -1)
						{
							perror("Apetuxe to kleisimo tis akris tou pipe");
							exit(-1);
						}
						pipeDesc[j] = -1;
					}
				}
			}

			/* Kathe paidi vlepei dup pipes, mia akrh apo to kathe ena */
			/* exit(work( pipeDesc + 2*i )); */
			if(work( pipeDesc[4*i] , pipeDesc[4*i + 3] ) == -1)
				exit(-1);

			/* kleinoun ta pipe pou xrisimopoihse to paidi */
			if(close(pipeDesc[4 * i]) == -1)
			{
				perror("Apetuxe to kleisimo tis akris diavasmatos tou pipe");
				exit(-1);
			}
			pipeDesc[4*i] = -1;
			if(close(pipeDesc[4 * i + 3]) == -1)
			{
				perror("Apetuxe to kleisimo tis akris grapsimatos tou pipe");
				exit(-1);
			}			
			pipeDesc[4*i+3] = -1;
			exit(0); /* to paidi termatise kanonika */
		case -1:
			perror("Den boresa na dhmiourghsw paidi - child.c");
			cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );
			return -1;

		default: /* pateras */
			/* Kleinei h akrh diavasmatos tou 1ou pipe */
			if(close(pipeDesc[4*i]) == -1)
			{
				perror("Apetuxe to kleisimo tis akris diavasmatos tou pipe");
				cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );
				return -1;
			}

			pipeDesc[4*i] = -1;

			/* Kleinei h akrh grapsimatos tou 2ou pipe */
			if( close(pipeDesc[4*i + 3]) == -1)
			{
				perror("Apetuxe to kleisimo tis akris grapsimatos tou pipe");
				cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );
				return -1;
			}

			pipeDesc[4*i + 3] = -1;

			/* Topothetei o pateras to paidi stin oura autwn pou kathontai */
			relaxQueue[i] = i; /* To i paidi kathetai */
		}

	}

/* --- pateras --- */
	/* Exoun dimiourghthei ola ta paidia kai meta analamvanei douleia o pateras */

	resultCounter = 0;
	working = 0; /* sthn arxh den douleiei kanenas kai oloi kathontai */
	relaxing = maxChildren;
	maxSize = 0;
	maxTime = 0;
	averageSize = 0;
	averageTime = 0;
	 

	while( resultCounter < maxUrls )
	{

	/* Moirasma douleias sta paidia apo ton patera kai eggrafh sto pipe */
		while( relaxing > 0 )
		{
			/* apothikeuetai to link pou epistrefetai apo tin
			oura, enw diagrafetai apo auth me tin removeQueue */
			if((link = removeQueue()) == NULL)
				break;

			/* apothikeuetai to megethos tou url */
			linkSize = strlen(link);

			/* Grafetai to mikos tou link sto pipe */
			/* relaxQueue[0], giati dinetai douleia panda ston 1o ths ouras pou kathetai */
			if(write( pipeDesc[4 * relaxQueue[0] + 1], &linkSize , sizeof(int) ) != sizeof(int) )
			{
				perror("Den boresa na grapsw to megethos tou link sto pipe");
				cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );
				return -1;
			}

			/* Grafetai to idio to link sto pipe */
			if(write( pipeDesc[4 * relaxQueue[0] + 1], link, linkSize * sizeof(char) ) != linkSize * sizeof(char) )
			{
				perror("Den boresa na grapsw to link sto pipe");
				cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );
				return -1;
			}

			/* Afou phre tin douleia, topotheteitai stin oura autwn pou douleuoun */
			workQueue[working] = relaxQueue[0];

			/* metraei posa paidia douleuoun pragmatika */
			working++;

			/* Vgainei apo tin oura pou kathetai me metatopish twn ipoloipwn mia thesh aristera */
			memmove( relaxQueue, relaxQueue + 1, (relaxing - 1) * sizeof(int) );

			relaxing--;
		}
		
		if (working == 0) 
		{
			printf("Den exw alla URLs gia crawl\n");
			break;
		}

	/* Diavasma apo ta pipes gia kathe paidi */
		
			/* Mazeuei douleia apo ton workQueue[0] */
		if( (bytesRead = read(pipeDesc[4 * workQueue[0] + 2], &linkSize, sizeof(int))) == 0 )
			break;
		else if(bytesRead != sizeof(int) )
		{
			perror("Den boresa na diavasw linkSize apo to pipe");
			cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );

			return -1;
		}

		if((results.url = (char *)malloc((linkSize + 1) * sizeof(char))) == NULL)
		{
			perror("Den boresa na desmeusw xwro gia to link");
			cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );

			return -1;
		}

		/*inkSize: xaraktires * sizeof(char) => bytes */
		if(read( pipeDesc[4 * workQueue[0] + 2], results.url, linkSize * sizeof(char) ) != linkSize * sizeof(char) )
		{
			perror("Den boresa na diavasw to link");
			cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );

			free(results.url);

			return -1;
		}

		results.url[linkSize] = '\0';

		/* diavazei apo tin akri diavasmatos tou 2ou pipe */
		if((bytesRead = read( pipeDesc[4 * workQueue[0] + 2], &(results.countImg), sizeof(int) )) == 0)
			break;
		else if(bytesRead != sizeof(int) )
		{
			perror("Den boresa na diavasw countImg apo to pipe");
			cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );

			return -1;
		}
	
		if(read( pipeDesc[4 * workQueue[0] + 2], &(results.countLink), sizeof(int) ) != sizeof(int))
		{
			perror("Den boresa na diavasw to plithos twn links");
			cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );

			return -1;
		}

		results.links = NULL;

	/* Mazeuei ta links tou result */
		for( j = 0; j < results.countLink; j++ )
		{
			if( (newLink = (ListNode *) malloc( sizeof(ListNode) )) == NULL )
			{
				perror("pateras: Den boresa na desmeusw xwro gia to neo link");
				cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );

				return -1;
			}

			/* diavasma tou megethous tou url */
			if(read( pipeDesc[4 * workQueue[0] + 2], &linkSize, sizeof(int) ) != sizeof(int))
			{
				perror("Den boresa na diavasw to megethos tou link");
				cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );
				free(newLink);

				return -1;
			}

			if((newLink->url = (char *)malloc((linkSize + 1) * sizeof(char))) == NULL)
			{
				perror("Den boresa na desmeusw xwro gia to link");
				cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );
				free(newLink);

				return -1;
			}

			/*inkSize: xaraktires * sizeof(char) => bytes */
			if(read( pipeDesc[4 * workQueue[0] + 2], newLink->url, linkSize * sizeof(char) ) != linkSize * sizeof(char) )
			{
				perror("Den boresa na diavasw to link");
				cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );

				free(newLink->url);
				free(newLink);

				return -1;
			}

			newLink->url[linkSize] = '\0';

			/* topothetish link stin arxh tis listas */
			newLink->next = results.links;

			results.links = newLink;
		
		}

		if(read( pipeDesc[4 * workQueue[0] + 2], &(results.pageSize), sizeof(int) ) != sizeof(int) )
		{
			perror("Den boresa na diavasw to megethos tis selidas");
			cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );

			return -1;			
		}


		if(read( pipeDesc[4 * workQueue[0] + 2], &urlTypeSize, sizeof(int) ) != sizeof(int) )
		{
			perror("Den boresa na diavasw to megethos tou urlType");
			cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );

			return -1;
		}


		if((results.urlType = (char *) malloc ( (urlTypeSize + 1) * sizeof(char))) == NULL)
		{
			perror("Den boresa na desmeusw xwro gia to urlType");
			cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );

			return -1;
		}


		if(read( pipeDesc[4 * workQueue[0] + 2], results.urlType, urlTypeSize * sizeof(char) ) != urlTypeSize * sizeof(char) )
		{
			perror("Den boresa na diavasw to urlType");
			cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );

			free(results.urlType);

			return -1;
		}

		results.urlType [urlTypeSize] = '\0';

		if(read( pipeDesc[4 * workQueue[0] + 2], &(results.time), sizeof(float)) != sizeof(float) )
		{
			perror("Den boresa na diavasw to time");
			cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );

			return -1;
		}
/* --- Exei parei ola ta apotelesmata tou paidiou --- */

		/* Afou diavastikan ta apotelesmata, benei to paidi sthn oura relaxQueue */
		relaxQueue[relaxing] = workQueue[0];

		relaxing++;

		/* afaireitai kai apo tin oura pou douleyoun */
		memmove( workQueue, workQueue + 1, (working - 1) * sizeof(int) );

		working--;

		/* fortwnei se dentro to arxiko url pou pire apo tin oura kai ta epimerous apotelesmata tou link */
		if(addTree( results.url, results.countImg, results.countLink, results.pageSize, results.urlType, results.time ) == -1)
		{
			cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );

			return -1;
		}

		/* Ypologismos tou pio ogkodous URL */
		if( results.pageSize > maxSize ) 
		{
			maxSize = results.pageSize;
			maxTime = results.time;
			maxUrl = results.url;
		}

		averageSize += results.pageSize;
		averageTime += results.time;

		resultCounter++;

		if( resultCounter >= maxUrls ) /* stamatame tin douleia, giati pragmatopoihthike o stoxos */
			break;


		/* Topothetish stin oura ta kainouria links pou efere to apotelesma */
		for( newLink = results.links; newLink != NULL; newLink = newLink->next )
		{
			if ( loadUrl(newLink->url) == -1 )
			{
				fprintf(stderr, "Den boresa na prosthesw url stin oura\n");
				cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );

				return -1;
			}
		}
	}

/* Teleiwse tin douleia o pateras kai arxizei na stelnei shmata s' ola ta paidia */
	for( i = 0; i < maxChildren; i++ )
	{
		if( kill( pids[i], SIGRTMIN+1 ) == -1 )
			fprintf(stderr,"Pateras: Den boresa na steilw sto paidi %d to sima SIGRTMIN+1\n", pids[i]);

	}

printf("\n");
printf("To pio ogkwdes URL htan to %s (%d bytes - %f seconds)\n", maxUrl, maxSize, maxTime);
printf("To meso megethos selidas einai: %.2f bytes\n", averageSize/(float)resultCounter);
printf("O mesos xronos prospelashs selidas einai: %.2f seconds\n", averageTime/(float)resultCounter);
printf("\n");
printTree( outputFile );

	cleanUp( maxChildren, pids, pipeDesc, workQueue, relaxQueue );
	return 0;
}


int main(int argc, char **argv)
{
	int i, maxChildren, maxUrls;
	char *inputDataFile = NULL, *startingURL = NULL, *numberOfProcesses = NULL, *numberOfURLs = NULL, *statsFile = NULL;
	char line[MAX_LINE], *url;
	FILE *inputFile, *outputFile;

	for( i = 0; i < argc; i++ )
	{
		if( strcmp( argv[i], "-f" ) == 0 )
		{
			/* elegxos gia na min dwsei o xrisths InputDataFile enw exeis hdh dwsei StartingURL */
			if( startingURL != NULL )
			{
				fprintf(stderr, "Exeis dwsei hdh StartingURL kai den boreis na dwseis InputDataFile\n");
				return -1;
			}

			if( inputDataFile != NULL)
			{
				fprintf(stderr, "To InputDataFile exei dothei hdh\n");
				return -1;
			}
			
			if( i+1 >= argc )/* den upaexei to orisma i+1*/
			{
				fprintf(stderr, "Edwses -f, alla den evales to InputDataFile\n");
				return -1;
			}

			inputDataFile = argv[i+1];
		}
		else if( strcmp( argv[i], "-u" ) == 0 )
		{
			/* elegxos gia na min dwsei o xrisths StartingURL enw exeis hdh dwsei InputDataFile */
			if( inputDataFile != NULL )
			{
				fprintf(stderr, "Exeis dwsei hdh InputDataFile kai den boreis na dwseis StartingURL\n");
				return -1;
			}

			if( startingURL != NULL )
			{
				fprintf(stderr, "To StartingURL exei dothei hdh\n");
				return -1;
			}

			if( i+1 >= argc )/* den upaexei to orisma i+1*/
			{
				fprintf(stderr, "Edwses -u, alla den evales to StartingURL\n");
				return -1;
			}

			startingURL = argv[i+1];
		}
		else if( strcmp( argv[i], "-p" ) == 0 )
		{
			if( numberOfProcesses != NULL )
			{
				fprintf(stderr, "Exeis dwsei hdh NumOfProcesses\n");
				return -1;
			}

			if( i+1 >= argc )/* den upaexei to orisma i+1*/
			{
				fprintf(stderr, "Edwses -p, alla den evales to NumOfProcesses\n");
				return -1;
			}

			numberOfProcesses = argv[i+1];
		}
		else if( strcmp( argv[i], "-n" ) == 0 )
		{
			if( numberOfURLs != NULL )
			{
				fprintf(stderr, "Exeis dwsei hdh numberOfURLs\n");
				return -1;
			}

			if( i+1 >= argc )/* den upaexei to orisma i+1*/
			{
				fprintf(stderr, "Edwses -n, alla den evales to numberOfURLs\n");
				return -1;
			}

			numberOfURLs = argv[i+1];
		}
		else if( strcmp( argv[i], "-s" ) == 0 )
		{
			if( statsFile != NULL )
			{
				fprintf(stderr, "Exeis hdh dwsei statsFile\n");
				return -1;
			}

			if( i+1 >= argc )/* den upaexei to orisma i+1*/
			{
				fprintf(stderr, "Edwses -s, alla den evales to StatsFile\n");
				return -1;
			}

			statsFile = argv[i+1];
		}
	}

	if( (inputDataFile == NULL) && (startingURL == NULL) )
	{
		fprintf(stderr, "Prepei na dwseis ena touxaliston apo ta InputDataFile kai StartingURL\n");
		return  -1;
	}

	if( numberOfProcesses == NULL )
	{
		fprintf(stderr, "Prepei na dwseis NumberOfProcesses\n");
		return -1;
	}

	if( numberOfURLs == NULL )
	{
		fprintf(stderr, "Prepei na dwseis NumberOfURLs\n");
		return -1;
	}

	if( statsFile == NULL )
	{
		fprintf(stderr, "Prepei na dwseis StatsFile\n");
		return -1;
	}

	if((maxChildren = atoi( numberOfProcesses )) <= 0)
	{
		fprintf(stderr,"To NumberOfProcesses prepei na einai thetikos akeraios\n");
		return -1;
	}
	
	if((maxUrls = atoi( numberOfURLs )) <= 0 )
	{
		fprintf(stderr, "To NumberOfURLs prepei na einai thetikos akeraios\n");
		return -1;
	}

	/* arxikopoihsh hashtable me megethos iso me to megisto arithmo urls giati
	to poly tosa url tha boun mesa */
	if( initializeHashtable( maxUrls ) == -1 ) /* periptwsh apotuxeias */
		return -1;

	if( inputDataFile != NULL )
	{
		if( (inputFile = fopen( inputDataFile, "r" )) == NULL )
		{
			fprintf(stderr, "Den boresa n' anoixw to arxeio %s gia diavasma\n", inputDataFile );
			return -1;
		}
			/* apothikeush sto line kata megethos MAX_LINE apo arxeio inputFile */
		while( fgets( line, MAX_LINE, inputFile ) != NULL )/* diavase epituxws mia grammh */
		{

			if ((strlen(line) != 0) && (line[strlen(line) - 1] == '\n'))
				line[strlen(line) - 1] = '\0';


			if((url = (char *)malloc( (strlen(line) + 1) * sizeof(char) )) == NULL)
			{
				perror("Den boresa na desmeusw xwro gia to url sth main");
				return -1;
			}
			strncpy(url, line, strlen(line));
			url[strlen(line)] = '\0';
			if( loadUrl( url ) == -1 )
			{
				free(url);
				return -1;
			}
		}
		
		/* Elenxei gia sfalmata sto arxeio kai an dhmiourgithei sfalma epistrefei mh midenikh timi */
		if(ferror( inputFile ))
		{
			fprintf(stderr, "Den boresa na diavasw apo to arxeio %s\n", inputDataFile );
			return -1;
		}

		if( fclose( inputFile ) == EOF )
		{
			fprintf(stderr, "Den boresa na kleisw to arxeio %s\n", inputDataFile );
			return -1;
		}
	}
	else
	{
		/* fortwnei to url pou edwse o xrhsths */
		if( loadUrl( startingURL ) == -1 )
				return -1;

	}

	if((outputFile = fopen( statsFile , "w" )) == NULL )
	{
		fprintf(stderr, "Den boresa n' anoixw to arxeio %s gia grapsimo\n", statsFile);
		return -1;
	}

	
	if(manage( maxChildren, maxUrls , outputFile ) == -1)
	{
		fclose( outputFile );
		return -1;
	}

	if( fclose(outputFile) == EOF )
	{
		fprintf(stderr, "Den boresa na kleisw to arxeio %s\n", statsFile);
		return -1;
	}

	return 0;  
}

