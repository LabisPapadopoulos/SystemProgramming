#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Domh dentrou gia taxinomish apotelesmatwn */
typedef struct TreeNode{
	char *url;
	int countImg;
	int countLink;
	int pageSize;
	char *urlType;
	float time;
	struct TreeNode *left;
	struct TreeNode *right;
}TreeNode;


TreeNode *tree = NULL;

/* xrish diplou deikth gia allagh tou dentrou */
int _addTree( TreeNode **tree, char *url, int countImg, int countLink, int pageSize, char *urlType, float time )
{
	int compareLinks;

	if( *tree == NULL )
	{
		if((*tree = (TreeNode *) malloc( sizeof(TreeNode) )) == NULL )
		{
			perror("Den boresa na desmeusw xwro gia ena TreeNode");
			return -1;
		}

		/* gemisma desmeumenou komvou */
		(*tree)->url = url;
		(*tree)->countImg = countImg;
		(*tree)->countLink = countLink;
		(*tree)->pageSize = pageSize;
		(*tree)->urlType = urlType;
		(*tree)->time = time;

		/* aristero kai dexi paidi, NULL */
		(*tree)->left = NULL;
		(*tree)->right = NULL;

		return 0;
	}
	else
	{
		if((compareLinks = strcmp((*tree)->url, url )) == 0 )
		{
			fprintf(stderr, "To url: %s uparxei hdh sto dentro\n", url );
			return -1;
		}
		else if( compareLinks > 0 ) /* (*tree)->url > url */
			/* eisagwgh sto aristero upodentro */
			return _addTree( &((*tree)->left), url, countImg, countLink, pageSize, urlType, time );
		else
			/* eisagwgh sto dexi upodentro */
			return _addTree( &((*tree)->right), url, countImg, countLink, pageSize, urlType, time );
	}
}


void _printTree( TreeNode *tree, FILE *outputFile )
{
	if( tree != NULL )
	{
		/* tupwnei olous tous mikroterous komvous */
		_printTree( tree->left, outputFile );

		fprintf( outputFile,"%s %d images %d links %d bytes %s %f seconds\n", tree->url, tree->countImg, tree->countLink, tree->pageSize, tree->urlType, tree->time );

		/* tupwnei olous tous megaluterous komvous */
		_printTree( tree->right, outputFile );

	}
}

int addTree( char *url, int countImg, int countLink, int pageSize, char *urlType, float time )
{
	return _addTree( &tree, url, countImg, countLink, pageSize, urlType, time );
}

void printTree( FILE *outputFile )
{
	_printTree( tree, outputFile );
}
