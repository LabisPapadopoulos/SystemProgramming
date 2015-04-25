#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>	/* times() */
#include <unistd.h>	/* sysconf() */
#include "cache.h"

#define MAX_BUF 128

int main(int argc, char** argv)
{
	FILE *fp;
	char rec[MAX_DATA], *FileWithPhones = NULL, *token;
	int i, NumOfRecords = 0;
	char buffer[MAX_BUF];

	double t1, t2, cpu_time;
	struct tms tb1, tb2;
	double ticspersec;


	if(argc < 5)
	{
		printf("Oxi arketa orismata!\n");
		printf("SYNTAX: %s -n <NumOfRecords> -f <FileWithPhones>\n", argv[0]);
		return -1;
	}
	
	for(i = 0; i < argc; i++ )
	{	
		if( strcmp("-n", argv[i]) == 0 )
		{
			/* Elegxos oti to i+1 einai egkuro orisma (oti uparxei) */
			if( i+1 < argc )
				NumOfRecords = atoi(argv[i+1]);
			else
			{
				printf("Den vrika noumero meta to -n\n");
				return -1;
			}	
		}
		else if( strcmp("-f", argv[i]) == 0 )
		{
			/* Elegxos oti to i+1 einai egkuro orisma (oti uparxei) */
			if( i+1 < argc )
				FileWithPhones = argv[i+1];
			else
			{
				printf("Den vrika arxeio meta to -f\n");
				return -1;
			}	
		}
	}

	if( NumOfRecords <= 0 )
	{
		printf("Den vrika egkuro NumOfRecords!\n");
		return -1;
	}

	if( FileWithPhones == NULL )
	{
		printf("Den vrika FileWithPhones!\n");
		return -1;
	}


	if ( ( fp = fopen(FileWithPhones, "r") ) == NULL )
	{
		printf("Den borese na anoixei to arxeio! %s\n", FileWithPhones);
		return -1;
	}

	ticspersec = (double) sysconf(_SC_CLK_TCK);	

	if(initialize( fp, NumOfRecords ) == -1 )
	{
		printf("Den boresa na arxikopoihsw tin CACHE!\n");
		return -1;
	}

	while(1)
	{
		printf("> ");
		/* Diavasma mias grammhs mexri na patithei to \n */
		if( fgets(buffer, MAX_BUF, stdin) == NULL )
		{
			printf("Den boresa na diavasw tin eisodo!\n");
			if(fclose(fp) == EOF)
				printf("Den boresa na kleisw to arxeio!\n");
			
			return -1;
		}
		
		if( (token = strtok(buffer," \t\n")) != NULL ) /* Vrike prwth lexh */
		{	
			if( (strcmp("r", token) == 0) || (strcmp("read", token) == 0) ) /* Anazhthsh egrafhs */
			{
				if( (token = strtok(NULL, " \t\n")) == NULL )
				{
					printf("Den vrika noumero pros anazhthsh!\n");
					continue;
				}
				
				t1 = (double) times(&tb1);

				switch ( search(atoi(token), rec ) )
				{
				case 1:
					printf("%s\n", rec);
					break;
				case 0:
					printf("Den vrika tin eggrafh\n");
					break;
				case -1:
					printf("Stravwse h douleia!\n");

					if(fclose(fp) == EOF)
						printf("Den boresa na kleisw to arxeio!\n");
						
					return -1;
				}
				
				t2 = (double) times(&tb2);

				cpu_time = (double) ((tb2.tms_utime + tb2.tms_stime) -
							(tb1.tms_utime + tb1.tms_stime));

				printf("Run time was %lf sec (REAL time) although we used the CPU for %lf sec (CPU time).\n",
						(t2 - t1) / ticspersec, cpu_time / ticspersec);

			}
			else if( (strcmp("l", token) == 0) || (strcmp("load", token) == 0) ) /* Fortwsh arxeiou */
			{
				if( (token = strtok(NULL, " \t\n" )) == NULL )
				{
					printf("Den vrika arxeio pros fortwsh!\n");
					continue;
				}

				if(fclose(fp) == EOF)
				{
					printf("Den boresa na kleisw to arxeio!\n");
					continue;					
				}
				if ( ( fp = fopen(token, "r") ) == NULL )
				{
					printf("Den borese na anoixei to arxeio %s!\n", token);
					continue;
				}

				t1 = (double) times(&tb1);

				/* Adeiazei h cache gia na doulepsei me to neo arxeio */
				finalize();

				if( initialize( fp, NumOfRecords ) == -1)
				{
				
					printf("Den boresa na arxikopoihsw tin CACHE!\n");
					
					if(fclose(fp) == EOF)
						printf("Den boresa na kleisw to arxeio!\n");
					
					return -1;
				}	

				t2 = (double) times(&tb2);

				cpu_time = (double) ((tb2.tms_utime + tb2.tms_stime) -
							(tb1.tms_utime + tb1.tms_stime));

				printf("Run time was %lf sec (REAL time) although we used the CPU for %lf sec (CPU time).\n",
						(t2 - t1) / ticspersec, cpu_time / ticspersec);
				
			}
			else if( (strcmp("e", token) == 0) || (strcmp("exit", token) == 0) ) /* termatismos programmatos */
				break;
		}
		
		
	}

	t1 = (double) times(&tb1);

	finalize();

	t2 = (double) times(&tb2);

	cpu_time = (double) ((tb2.tms_utime + tb2.tms_stime) -
				(tb1.tms_utime + tb1.tms_stime));

	printf("Run time was %lf sec (REAL time) although we used the CPU for %lf sec (CPU time).\n",
			(t2 - t1) / ticspersec, cpu_time / ticspersec);

	if(fclose(fp) == EOF)
	{
		printf("Den boresa na kleisw to arxeio!\n");
		return -1;
	}
	
	return 0;
}
