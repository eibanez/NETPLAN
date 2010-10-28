
// NOTE: I just quickly put this class together to make the old C code work
//       in C++ class form.  This could be done FAR more elegantly.

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct lists {
	int index;
	struct lists *parent;
	struct lists *child;
}
list;

class CLinkedList {
	public:
		CLinkedList(void);
		~CLinkedList(void);

		void  insert (list *node, int x);
		list* del (list *node);
};

