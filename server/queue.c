#include "queue.h"
#include <stdlib.h>

void queue_push(struct task task, struct node *head)
{
	struct node *node = malloc(sizeof(struct node));
	node->task = task;
	if (head == NULL)
		head = node;
}

struct task queue_pop(struct node *tail)
{
	struct task ret = node->task;
	
}
