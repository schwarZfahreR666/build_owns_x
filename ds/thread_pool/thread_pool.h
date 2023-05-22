#ifndef THREAD_POOL

#define THREAD_POOL

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include "queue.h"

typedef struct {
    pthread_mutex_t lock; //锁
    sem_t m_sem;          //是否有任务需要处理
    pthread_t *threads;
    Queue* task_queue;
    int thread_count;
    int task_count;

    int status; //线程池状态，0为结束，1为启动
} ThreadPool;

void destory_threadpool(ThreadPool * pool);
void thread_run(ThreadPool * pool);
void* threadpool_worker(void* arg);
ThreadPool* threadpool_create(int thread_count,int task_count);
int append_task(ThreadPool* pool,Task* task);

#endif  //THREAD_POOL