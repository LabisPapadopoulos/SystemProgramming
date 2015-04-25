#include <pthread.h>

pthread_mutex_t mtx;

int initializeSemaphore( void )
{
	/* arxikopoihsh tou semaphorou */
	if( pthread_mutex_init( &mtx, NULL ) != 0 )
		return -1;

	return 0;
}


int up( void ) /* +1 */
{
	if ( pthread_mutex_unlock( &mtx ) != 0 )
		return -1;

	return 0;
}


int down( void ) /* -1 */
{
	if ( pthread_mutex_lock( &mtx ) != 0 )
		return -1;

	return 0;
}

int finalizeSemaphore( void )
{
	if( pthread_mutex_destroy( &mtx ) != 0 )
		return -1;

	return 0;
}
