#ifndef SERVER_THREADPOOL_HPP
#define SERVER_THREADPOOL_HPP
#include <pthread.h>

int allocate_pool(int thread_num);
int finish_pool();
int push_task();

#endif
