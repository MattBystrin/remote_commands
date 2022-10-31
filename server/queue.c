#include <stdlib.h>
#include <stdbool.h>

#include "queue.h"

/* Insert after head */
void queue_push(task_t task, struct node *head)
{
	struct node *new = malloc(sizeof(struct node));
	struct node *next = head->next;
	head->next = new;
	next->prev = new;
	new->prev = head;
	new->next = next;

	new->task = task;
}

/* Remove before head and return task */
task_t queue_pop(struct node *head)
{
	task_t ret;
	struct node *prev = head->prev;
	ret = prev->task;
	prev->prev->next = head;
	head->prev = prev->prev;
	free(prev);
	return ret;
}

struct node *queue_init()
{
	struct node *head = malloc(sizeof(struct node));
	head->next = head;
	head->prev = head;
	head->task.function = NULL;
	head->task.args = NULL;
	return head;
};

bool queue_empty(struct node *head)
{
	return head->next == head;
}

void queue_delete(struct node *head)
{
	struct node *next = head->next;
	while(next != head) {
		struct node *tmp = next;
		next = next->next;
		free(tmp);
	}
	free(head);
}
