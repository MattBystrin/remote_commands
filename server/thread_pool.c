#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>

#include "queue.h"
#include "thread_pool.h"
#include "messages.h"

struct thread_pool {
	pthread_t *threads;
	int num;
	pthread_mutex_t mtx;
	pthread_cond_t condv;
	bool running; /* Must be changed under mutex */
	struct node *queue;
	int pending; /* Also must be changed under mutex */
};

static struct thread_pool pool = {
	.threads = NULL,
	.num = 0,
	.mtx = 0,
	.condv = 0,
	.queue = NULL,
	.pending = 0
};

void *work_func(void *arg)
{
	while(1) {
		pthread_mutex_lock(&pool.mtx);

		while(pool.running == true && queue_empty(pool.queue))
			pthread_cond_wait(&pool.condv, &pool.mtx);

		if (pool.running == false && queue_empty(pool.queue)) {
			pthread_mutex_unlock(&pool.mtx);
			return arg;
		}
		/* Critical section */
		task_t task = queue_pop(pool.queue);
		pthread_mutex_unlock(&pool.mtx);

		exec_task(task);

		pthread_mutex_lock(&pool.mtx);
		pool.pending--; /* Reset stricly after the task is done */
		pthread_mutex_unlock(&pool.mtx);
	}
}

int push_task(task_t task)
{
	pthread_mutex_lock(&pool.mtx);
	queue_push(task, pool.queue);
	pool.pending++;
	pthread_mutex_unlock(&pool.mtx);
	pthread_cond_signal(&pool.condv);
	return 0;
}


bool tasks_ready()
{
	pthread_mutex_lock(&pool.mtx);
	bool ret = pool.pending == 0;
	pthread_mutex_unlock(&pool.mtx);
	return ret;
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
	printf("Allocating pool of %d threads\n", num);
	assert(pool.num == 0);
	pool.num = num;
	pool.running = true;
	pool.threads = malloc(num * sizeof(pthread_t));
	if (!pool.threads)
		err(0, "thread array allocation failed");
	pool.queue = queue_init();
	/* Allocate mutex */
	int ret = pthread_mutex_init(&pool.mtx, NULL);
	if (ret)
		err(ret, "mutex init failed");
	/* Allocate condvar */
	ret = pthread_cond_init(&pool.condv, NULL);
	if (ret)
		err(ret, "condv init failed");
	/* Create threads */
	for (int i = 0; i < num; i++) {
		ret = pthread_create(pool.threads + i, NULL, work_func, NULL);	
		if (ret)
			err(ret, "thread creation failed");
	}
	return 0;
}
