#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h> /* signals */
#include <unistd.h> /* fork */
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "child.h"
#include "results.h"

#define BUFFER_SIZE 1024 * 1024

/* man signal
typedef void (*sighandler_t)(int);

sighandler_t signal(int signum, sighandler_t handler); */

int kill(pid_t , int );

/* Kaleitai otan theloume na termatisoume */
void signalHandler( int signum )
{
	if( signum == SIGRTMIN+1 )
	{
		/* Katharizei oti uparxei */

		/* getppid(): Epistrefei to pid tou patera */
		/* To paidi tha steilei shma ston patera oti teleiwse */
		if( kill( getppid(), SIGRTMIN+2 ) == -1 )
			perror("Den boresa na steilw shma ston patera");

		exit(0);
	}
}


void freeList( ListNode *links )
{
	if(links == NULL)
		return;
	free(links->url);

	/* Anadromikh klish tis freeList gia 
	tin diagrafh twn epomenwn komvwn tis listas */
	freeList(links->next);

	/* Stin epistrofh tis anadromhs svinetai o
	trexontas komvos */
	free(links);
}

/* Orismata me deiplo deikth, gia na boresei na 
perasei deikth me anafora san orisma */
int urlInfo( char *url, char **protocol, char **host, char **path, char **urlType )
{
	char *start, *end, *temp;

	*protocol = NULL;
	*host = NULL;
	*path = NULL;
	*urlType = NULL;

	/* euresh protokollou */
	if (strstr(url, "http://") != NULL) /* to URL xekinai me http:// */
	{
		if((*protocol = (char *)malloc((strlen("http://") + 1) * sizeof(char) )) == NULL )
		{
			perror("Den boresa na desmeusw xorw gia to protokolo");
			return -1;
		}
		strcpy(*protocol, "http://");
		(*protocol)[strlen("http://")] = '\0';
	}
	else if (strstr(url, "https://") != NULL) /* to URL xekinai me https:// */
	{
		if((*protocol = (char *)malloc((strlen("https://") + 1) * sizeof(char) )) == NULL )
		{
			perror("Den boresa na desmeusw xorw gia to protokolo");
			return -1;
		}
		strcpy(*protocol, "https://");
		(*protocol)[strlen("https://")] = '\0';
	}
	else
	{
		/* Se periptwsh pou den vrethei tipota, desmaeush xwrou 1os xarakthra */
		if((*protocol = (char *)malloc(sizeof(char) )) == NULL )
		{
			perror("Den boresa na desmeusw xorw gia to protokolo");
			return -1;
		}
		(*protocol)[0] = '\0';
	}

	/* euresh host */
	start = url + strlen(*protocol);
	if ((end = strstr(start, "/")) == NULL)
	{
		if((*host = (char *)malloc(sizeof(char) )) == NULL )
		{
			perror("Den boresa na desmeusw xorw gia to host");
			free(*protocol);
			*protocol = NULL;
			return -1;
		}
		(*host)[0] = '\0';
	}
	else
	{
		if((*host = (char *)malloc( ((end - start) + 1 ) * sizeof(char) )) == NULL )
		{
			perror("Den boresa na desmeusw xorw gia to host");
			free(*protocol);
			*protocol = NULL;
			return -1;
		}
		strncpy( *host , start , (end - start) );
		(*host)[end - start] = '\0';
	}

	/* euresh path */
	start = url + strlen(*protocol) + strlen(*host);
	end = start + strlen(start);

	if((temp = strstr(start, "?")) != NULL) /* vrike to ? */
		end = temp;

	if((temp = strstr(start, "#")) != NULL) /* vrike to # */
		end = temp;

	/* psaxnei antistrofa na vrei to 1o "/"
	pou tha shmainei oti termatizei to path */
	for( temp = end; temp > start; temp-- )
		if( *temp == '/' )
			break;

	/* +2 gia na bei kai to teleutaio / sto path */
	if((*path = (char *)malloc( ((temp - start) + 2 ) * sizeof(char) )) == NULL )
	{
		perror("Den boresa na desmeusw xorw gia to path");
		free(*protocol);
		*protocol = NULL;
		free(*host);
		*host = NULL;
		return -1;
	}
	strncpy( *path , start , (temp - start) + 1 );
	(*path)[temp - start + 1] = '\0';

	for( ; temp < end; temp++ )
		if( *temp == '.' )
			break;

	if( end != temp )
	{
		if((*urlType = (char *)malloc( ((end - temp) + 1 ) * sizeof(char) )) == NULL )
		{
			perror("Den boresa na desmeusw xorw gia to urlType");
			free(*protocol);
			*protocol = NULL;
			free(*host);
			*host = NULL;
			free(*path);
			*path = NULL;
			return -1;
		}
		strncpy( *urlType , temp , (end - temp));
		(*urlType)[end - temp] = '\0';
	}
	else /* Den vrhke epektash arxeiou */
	{
		if((*urlType = (char *)malloc( (strlen("Directory") + 1 ) * sizeof(char) )) == NULL )
		{
			perror("Den boresa na desmeusw xorw gia to urlType");
			free(*protocol);
			*protocol = NULL;
			free(*host);
			*host = NULL;
			free(*path);
			*path = NULL;
			return -1;
		}
		strcpy( *urlType , "Directory");
		(*urlType)[strlen("Directory")] = '\0';		
	}
	return 0;
}

int crawl( char *url, Results *results )
{
	/* to child tou child */
	pid_t wget;
	time_t beginTime, endTime;
	int status, pipeDesc[2];
	char buffer[BUFFER_SIZE], *parse, *linkStart, *linkEnd;
	char *protocol, *host, *path;
	ListNode *newLink;

	printf("Crawling %s\n", url);
	
	results->url = url;

	if( pipe(pipeDesc) == -1 )
	{
		perror("Den borese na dimiourgithei to pipe");
		return -1;
	}
	switch(wget = fork())
	{
	case 0: /* To paidi-wget*/
		/* Eimaste sto wget paidi kai den xreiazetai h
		akrh diavasmatos */
		if( close(pipeDesc[0]) == -1 )
		{
			perror("Apotuxeia sto kleisomo tis akris diavasmatos tou pipe");
			close(pipeDesc[1]);

			return -1;
		}
		/* 0: stdin
		   1: stdout
		   2: stderr */

		/* Antigrafei tin akri grapsimatos tou pipe (pipeDesc[1]) 
		sto stdout(1) - epeidh xrisimopoieitai h wget */

		if( dup2( pipeDesc[1] , 1) == -1)
		{
			perror("Apotuxeia sto dup2");
			close(pipeDesc[0]);
			close(pipeDesc[1]);

			return -1;
		}

		/* Epeidh h wget pou tha klithei meta me to execlp, tha prospathisei
		na grapsei oti vrei sto pipe, to opoio exei sugkekrimeno megethos
		se antithesh me to stdout pou einai aperioristo. Opote an to pipe gemisei
		h wget tha perimenei mexri to child na diavasei merika wste na adeiasei xwros.
		Omws epeidh to child kanei wait kai meta diavazei, exei ws apotelesma na kollaei.
		H sunatish fcntl thetei to NONBLOCK flag (F_SETFL) tou pipe se 1, opote otan
		gemisei to pipe h wget den perimenei allo kai den grafei ta upoloipa kai sunexizei */
		if (fcntl(pipeDesc[1], F_SETFL, O_NONBLOCK) == -1) 
		{
			perror("Apotuxeia sto fcntl");
			close(pipeDesc[0]);
			close(pipeDesc[1]);

			return -1;
		}
		
			/* lista me orismata tis wget */
			/* -q: den tha vgalei minimata stin consola */
			/* -O - : To output tha einai to stdout(-),
			dhladh to pipe */
		execlp("wget", "wget", "-q", "-O", "-", "--", url, NULL);
		perror("Yparxei provlima, den ektelestike h wget");
		close(pipeDesc[0]);
		close(pipeDesc[1]);
		return -1;
	case -1:
		perror("Den borese na dimiourgithei to paidi-wget");
		close(pipeDesc[0]);
		close(pipeDesc[1]);
		return -1;
	default: /* Pateras */
		if((beginTime = time(NULL)) == -1)
		{
			perror("Den boresa na vrw ton xrono enarxhs");
			return -1;
		}
		/* Eimaste ston patera, opote den xreiazetai to pipe grapsimatos anoixto */
		if( close(pipeDesc[1]) == -1 )
		{
			perror("Apotuxeia sto kleisomo tis akris grapsimatos tou pipe");
			close(pipeDesc[0]);

			return -1;
		}		

		/* O pateras perimenei na teleiwsei to paidi */
		if( wait( &status ) == -1)
		{
			perror("Provlima me tin wait tou paidiou!");
			return -1;
		}

		if( (results->pageSize = read(pipeDesc[0], buffer, (BUFFER_SIZE - 1) * sizeof(char) )) == -1 ) /* uparxei sfalma */
		{
			perror("Den boresa na diavasw to buffer");
			return -1;
		}

		buffer[BUFFER_SIZE - 1] = '\0';

		results->countImg = 0;

		/* arxikopoihsh stin 1h image pou 
		emberiexetai mesa sto buffer */			    /* proxwraei sto epomeno 1o image pou tha vrei */
		for (parse = strstr(buffer, "<img"); parse != NULL; parse = strstr(parse, "<img") )
		{
			/* proxwraei o deikths meta to string <img */
			parse += strlen("<img") * sizeof(char);

			results->countImg++;
		}

		/* Euresh links */
		results->countLink = 0;
		/* arxh tis listas me ta links */
		results->links = NULL;

		if(urlInfo( url, &protocol, &host, &path, &(results->urlType) ) == -1)
			return -1;

		/* Kathe fora paei se kathe epomeno href=" */
								/* Proxwraei mexri na vrei href=" */
		for (parse = strstr(buffer, "href=\""); parse != NULL; parse = strstr(parse, "href=\"") )
		{
			
			/* O deikths tha deixnei sto h tou http://... */
			linkStart = parse + strlen("href=\"") * sizeof(char);
			linkEnd = strstr(linkStart, "\"");

			if( ( newLink = (ListNode *)malloc( sizeof(ListNode) )) == NULL )
			{
				perror("Den boresa na desmeusw komvo gia tin apothikeush enos link");
				return -1;
			}
			
			if (strncmp(linkStart, "http", strlen("http")) == 0) /* einai plhres url, de tha bei tipota allo brosta */
			{
				/* Desmeush xwrou gia ena link */
				/* h diafora linkEnd - linkStart einai se xaraktires (posoi xaraktires einai) */
											    /* sin 1 xaraktira gia to \0 */
				if(( newLink->url = (char *) malloc ( (linkEnd - linkStart + 1) * sizeof(char))) == NULL)
				{
					perror("Den boresa na desmeusw xoro gia ena link");
					free(newLink);
					return -1;
				}

				/* antigrafh tou link sto url apo to linkStart */
				strncpy( newLink->url , linkStart , linkEnd - linkStart );

				/* Proxwraei o pinakas url osa bytes xreiazetai mexri na ftasei sto telos
				kai sto periexomeno ekeinhs tis theshs benei '\0' */
				*(newLink->url + (linkEnd - linkStart)) = '\0';
			}
			else if (strncmp(linkStart, "//", strlen("//")) == 0) /* einai url xwris protokolo, tha bei brosta */
			{
				linkStart += strlen("//");
				/* Desmeush xwrou gia ena link */
				/* h diafora linkEnd - linkStart einai se xaraktires */
											    								/* sin 1 xaraktira gia to \0 */
				if(( newLink->url = (char *) malloc (( strlen(protocol) + linkEnd - linkStart + 1) * sizeof(char))) == NULL)
				{
					perror("Den boresa na desmeusw xoro gia ena link");
					free(newLink);
					return -1;
				}

				/* antigrafh tou protocol sto url */
				strncpy( newLink->url , protocol , strlen(protocol) );

				/* antigrafh kai tou telikou url apo to linkStart sto url tou komvou */
				strncpy( newLink->url + strlen(protocol), linkStart , linkEnd - linkStart );

				/* Proxwraei o pinakas url osa bytes xreiazetai mexri na ftasei sto telos
				kai sto periexomeno ekeinhs tis theshs benei '\0' */
				*(newLink->url + strlen(protocol) + strlen(":") + (linkEnd - linkStart)) = '\0';
			}
			else if (strncmp(linkStart, "/", strlen("/")) == 0) /* einai root url, tha bei mono host brosta kai protokolo */
			{
				/* Desmeush xwrou gia ena link */
				/* h diafora linkEnd - linkStart einai se xaraktires */
											    								/* sin 1 xaraktira gia to \0 */
				if(( newLink->url = (char *) malloc (( strlen(protocol) + strlen(host) + linkEnd - linkStart + 1) * sizeof(char))) == NULL)
				{
					perror("Den boresa na desmeusw xoro gia ena link");
					free(newLink);
					return -1;
				}

				/* antigrafh tou protocol sto url */
				strncpy( newLink->url , protocol , strlen(protocol) );

				/* antigrafh tou host sto telos tou url */
				strncpy( newLink->url + strlen(protocol), host , strlen(host) );

				/* antigrafh kai tou telikou url apo to linkStart sto url tou komvou */
				strncpy( newLink->url + strlen(protocol) + strlen(host) , linkStart , linkEnd - linkStart );

				/* Proxwraei o pinakas url osa bytes xreiazetai mexri na ftasei sto telos
				kai sto periexomeno ekeinhs tis theshs benei '\0' */
				*(newLink->url + strlen(protocol) + strlen(host) + (linkEnd - linkStart)) = '\0';
			}
			else /* Einai relative url, exartatai apo to trexon url */
			{
				/* Desmeush xwrou gia ena link */
				/* h diafora linkEnd - linkStart einai se xaraktires */
											    								/* sin 1 xaraktira gia to \0 */
				if(( newLink->url = (char *) malloc (( strlen(protocol) + strlen(host) + strlen(path) + linkEnd - linkStart + 1) * sizeof(char))) == NULL)
				{
					perror("Den boresa na desmeusw xoro gia ena link");
					free(newLink);
					return -1;
				}

				/* antigrafh tou protocol sto url */
				strncpy( newLink->url , protocol , strlen(protocol) );

				/* antigrafh tou host sto telos tou url */
				strncpy( newLink->url + strlen(protocol), host , strlen(host) );

				/* antigrafh tou path sto telos tou url */				
				strncpy( newLink->url + strlen(protocol) + strlen(host), path , strlen(path) );

				/* antigrafh kai tou telikou url apo to linkStart sto url tou komvou */
				strncpy( newLink->url + strlen(protocol) + strlen(host) + strlen(path) , linkStart , linkEnd - linkStart );

				/* Proxwraei o pinakas url osa bytes xreiazetai mexri na ftasei sto telos
				kai sto periexomeno ekeinhs tis theshs benei '\0' */
				*(newLink->url + strlen(protocol) + strlen(host) + strlen(path) + (linkEnd - linkStart)) = '\0';
			}

			/* 1) O neos komvos deixnei ekei pou edeixne to links (stin arxh NULL) */
			newLink->next = results->links;
			
			/* 2) to links deixnei ston neo komvo (to links menei panda
			stin arxh tis sundedemenhs listas) */
			results->links = newLink;

			/* proxwraei o deikths meta to autaki (") pou teleiwnei to href */
			parse = linkEnd + strlen("\"") * sizeof(char);

			results->countLink++;

		}

		if( close(pipeDesc[0]) == -1 )
		{
			perror("Apotuxeia sto kleisomo tis akris diavasmatos tou pipe");
			/* Katharismos listas se periptwsh apotuxeias */
			freeList(results->links);
			return -1;
		}	
			
		if((endTime = time(NULL)) == -1)
		{
			perror("Den boresa na vrw ton xrono enarxhs");
			freeList(results->links);
			return -1;
		}
		results->time = difftime(endTime, beginTime);

		return status;
	}
}

/* Benei ena zeugari akeraiwn pipe wste na boroun na epikoinwnoun
o manager me to child */
int work( int input, int output )
{
	int bytesRead, linkSize, urlTypeSize;
	char *link;
	Results results;
	ListNode *newLink;

	/* Otan to paidi parei shma SIGRTMIN+1 apo ton patera, tote
	ektelei tin signalHandler */
	if( signal( SIGRTMIN+1, signalHandler ) == SIG_ERR )
	{
		perror("Den boresa na thesw tin sunartish signalHandler gia xeirismo tou simatos SIGRTMIN+1");
		return -1;
	}


	while( 1 )
	{

		/* Diavazei enan akeraio gia to mikos tou url */
		/* Periptwsh pou to bytesRead einai 0, den uparxei link gia diavasma */
		if ( (bytesRead = read( input, &linkSize, sizeof(int)) ) == 0 )
			break;/* stamata na douleueis */
		else if( bytesRead != sizeof(int) )
		{
			perror("Den boresa na diavasw to mikos tou url");
			return -1;
		}

		/* dunamiki desmeush tou url kata linkSize */
		if( (link = (char *) malloc((linkSize + 1) * sizeof(char))) == NULL )
		{
			perror("Den boresa na desmeusw xoro gia to megethos tou url");
			return -1;
		}

		/* diavasma tou url apo to pipeDesc[0] kai apothikeush sto link */
		if( read( input , link , linkSize * sizeof(char) ) == -1 )
		{
			perror("Den boresa na diavasw to url");
			free(link);
			return -1;
		}

		/* termatismos string */
		link[linkSize] = '\0';
		
		/* crawl to url kai ta apotelesmata tha grafontai sto pipeDesc[1] */
		if (crawl( link, &results ) == -1 )
		{
			fprintf(stderr, "Den boresa na kanw crawl to url\n");
			free(link);
			return -1;
		}

		linkSize = strlen(results.url);
		if(write( output, &linkSize, sizeof(int)) != sizeof(int))
		{
			perror("Den borese na graftei to megethos tou link sto pipe");

			return -1;
		}

		if(write( output, results.url , linkSize * sizeof(char) ) != linkSize * sizeof(char))
		{
			perror("Den borese na graftei to link sto pipe");
		
			return -1;
		}


		if(write( output, &(results.countImg), sizeof(int) ) != sizeof(int))
		{
			perror("Den borese na graftei to countImg sto pipe");

			free(link);
			freeList(results.links);
			return -1;
		}
		if(write( output, &(results.countLink), sizeof(int) ) != sizeof(int))
		{
			perror("Den borese na graftei to countLink sto pipe");

			free(link);
			freeList(results.links);
			return -1;
		}

		/* Prospelash tis listas me ta url kai grapsimo ena ena sto pipe */
		for( newLink = results.links; newLink != NULL; newLink = newLink->next )
		{
			linkSize = strlen(newLink->url);
			if(write( output, &linkSize, sizeof(int)) != sizeof(int))
			{
				perror("Den borese na graftei to megethos tou link sto pipe");

				free(link);
				freeList(results.links);
				return -1;
			}

			if(write( output, newLink->url , linkSize * sizeof(char) ) != linkSize * sizeof(char))
			{
				perror("Den borese na graftei to link sto pipe");
				
				free(link);
				freeList(results.links);
				return -1;
			}
		}


		if(write( output, &(results.pageSize), sizeof(int) ) != sizeof(int))
		{
			perror("Den borese na graftei to pageSize sto pipe");

			free(link);
			freeList(results.links);
			return -1;
		}

		urlTypeSize = strlen(results.urlType);
		/* grafei arxika to mikos tou url type */
		if(write( output, &urlTypeSize, sizeof(int) ) != sizeof(int))
		{
			perror("Den borese na graftei to urlTypeSize sto pipe");

			free(link);
			freeList(results.links);
			return -1;
		}

		if(write( output, results.urlType, urlTypeSize * sizeof(char) ) != urlTypeSize * sizeof(char))
		{
			perror("Den borese na graftei to urlType sto pipe");

			free(link);
			freeList(results.links);
			return -1;
		}
		if(write( output, &(results.time), sizeof(float) ) != sizeof(float))
		{
			perror("Den borese na graftei to time sto pipe");

			free(link);
			freeList(results.links);
			return -1;
		}

	}
	return 0;
}

