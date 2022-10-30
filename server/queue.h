#ifndef SERVER_QUEUE_HPP
#define SERVER_QUEUE_HPP

#include "thread_pool.h"

struct node {
	struct node *next;
	struct node *prev;
	task_t task;
};

void queue_push(task_t task, struct node *head);
struct task queue_pop(struct node *node);

#endif
