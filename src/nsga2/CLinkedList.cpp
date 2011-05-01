#include "CLinkedList.h"

CLinkedList::CLinkedList(void) {}

CLinkedList::~CLinkedList(void) {}

/* Insert an element X into the list at location specified by NODE */
void CLinkedList::insert (list *node, int x) {
	list *temp;
	if (node==NULL) {
		printf("\n Error, asked to enter after a NULL pointer, hence exiting\n");
		exit(1);
	}
	temp = (list *)malloc(sizeof(list));
	temp->index = x;
	temp->child = node->child;
	temp->parent = node;
	
	if (node->child != NULL)
		node->child->parent = temp;
	
	node->child = temp;
}

/* Delete the node NODE from the list */
list* CLinkedList::del (list *node) {
	list *temp;
	if (node==NULL) {
		printf("\n Error, asked to delete a NULL pointer, hence exiting\n");
		exit(1);
	}
	temp = node->parent;
	temp->child = node->child;
	
	if (temp->child!=NULL)
		temp->child->parent = temp;
	
	free (node);
	return (temp);
}
