#ifndef RESULTS_H
#define RESULTS_H

/* Domi apothikeushs twn URLs se lista */
typedef struct ListNode{
	char *url;
	struct ListNode * next;
}ListNode;

/* apotelesmata apo ena crawl */
typedef struct {
	char *url;
	int countImg;
	int countLink;
	ListNode *links;
	int pageSize;
	char *urlType;	
	float time;
}Results;

#endif
