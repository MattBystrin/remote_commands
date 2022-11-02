#ifndef SERVER_QUEUE_HPP
#define SERVER_QUEUE_HPP

#include <stdbool.h>
#include "thread_pool.h"

struct node {
	struct node *next;
	struct node *prev;
	task_t task;
};

struct node *queue_init();
void queue_push(task_t task, struct node *head);
task_t queue_pop(struct node *head);
void queue_delete(struct node *head);
int queue_size(struct node *head);
bool queue_empty(struct node *head);

#endif
