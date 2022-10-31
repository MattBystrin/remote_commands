#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

#include "thread_pool.h"
#include "queue.h"

struct thread_pool {
	pthread_t *threads;
	int num;
	pthread_mutex_t mtx;
	pthread_cond_t condv;
	/* Must be changed under mutex */
	bool running;
	struct node *queue;
};

static struct thread_pool pool = {
	.threads = NULL,
	.num = 0,
	.mtx = 0,
	.condv = 0,
	.queue = NULL
};

static int availtasks = 0;

void inside_func(void *arg)
{
	int *casted = (int *)arg;
	printf("Getted arg %d\n", *casted);
}

static int intarg = 7;

task_t example_task = {
	.function = inside_func,
	.args = &intarg,
};

void *work_func(void *arg)
{
	while(1) {
		pthread_mutex_lock(&pool.mtx);
		/* */
		while(pool.running == true && queue_empty(pool.queue))
			pthread_cond_wait(&pool.condv, &pool.mtx);

		if (pool.running == false && queue_empty(pool.queue)) {
			pthread_mutex_unlock(&pool.mtx);
			return arg;
		}
		/* Critical section */
		task_t task = queue_pop(pool.queue);
		pthread_mutex_unlock(&pool.mtx);
		/* Non-critial section */
		exec_task(task);
	}
}

int push_task(task_t task)
{
	pthread_mutex_lock(&pool.mtx);
	queue_push(task, pool.queue);
	pthread_mutex_unlock(&pool.mtx);
	pthread_cond_signal(&pool.condv);
	return 0;
}

int finish_pool()
{
	pthread_mutex_lock(&pool.mtx);
	pool.running = false;
	pthread_mutex_unlock(&pool.mtx);
	pthread_cond_broadcast(&pool.condv);

	for (int i = 0; i < pool.num; i++)
		pthread_detach(pool.threads[i]);
	free(pool.threads);

	pthread_cond_destroy(&pool.condv);
	pthread_mutex_destroy(&pool.mtx);
	queue_delete(pool.queue);
	printf("Pool finished\n");
	return 0;
}

int allocate_pool(int num)
{
	printf("Allocatin pool of %d threads\n", num);
	if (pool.num != 0) {
		printf("Warning: thread pool already allocated\n");
		return -1;
	}
	pool.num = num;
	pool.running = true;
	pool.threads = malloc(num * sizeof(pthread_t));
	pool.queue = queue_init();
	/* Allocate mutex */
	int ret = pthread_mutex_init(&pool.mtx, NULL);
	if (ret) {
		printf("Error creating mutex\n");
		return -1;
	}
	/* Allocate condvar */
	ret = pthread_cond_init(&pool.condv, NULL);
	if (ret) {
		printf("Error creating condvar\n");
		return -1;
	}
	/* Create threads */
	for (int i = 0; i < num; i++) {
		ret = pthread_create(pool.threads + i, NULL, work_func, NULL);	
		if (ret) {
			printf("Error %d creating thread %d\n", ret, i);
			return ret;
		}
	}
	return 0;
}
