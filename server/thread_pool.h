#ifndef SERVER_THREADPOOL_H
#define SERVER_THREADPOOL_H
#include <pthread.h>

int allocate_pool(int thread_num);
int finish_pool();
int push_task();

struct thread_pool;

typedef struct {
	void (*function) (void *);
	void *args;
} task_t;

typedef task_t handle_t;
#define exec_task(x) x.function(x.args)

#endif
