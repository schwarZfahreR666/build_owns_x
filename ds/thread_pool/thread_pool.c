#include "thread_pool.h"



void destory_threadpool(ThreadPool * pool){
    free(pool->task_queue);
    free(pool->threads);
    free(pool);
}

void thread_run(ThreadPool * pool){
    while(pool->status){
        sem_wait(&pool->m_sem);
        pthread_mutex_lock(&pool->lock);
        if(is_empty(pool->task_queue)){
            pthread_mutex_unlock(&pool->lock);
            continue;
        }
        
        Task* task = (Task *)pop(pool->task_queue);
        pthread_mutex_unlock(&pool->lock);
        task->func(task->arg);
    }
}

void* threadpool_worker(void* arg){
    ThreadPool* pool = (ThreadPool*)arg;
    thread_run(pool);
    return pool;
}

ThreadPool* threadpool_create(int thread_count,int task_count){
    ThreadPool* pool;

    if((pool = (ThreadPool*) malloc(sizeof(ThreadPool))) == NULL){
        return NULL;
    }

    pool->thread_count = 0;
    pool->task_count = 0;
    pool->status = 1;

    pool->task_queue = create_queue(task_count);

    pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * thread_count);

    pthread_mutex_init(&pool->lock,NULL);
    sem_init(&pool->m_sem, 0, 0);

    for(int i=0;i<thread_count;i++){
        if(pthread_create(&pool->threads[i],NULL,threadpool_worker,(void *)pool) != 0){
            printf("pthread create failed\n");
            destory_threadpool(pool);
            return NULL;
        }

        pool->thread_count++;
    }

    return pool;
}

int append_task(ThreadPool* pool,Task* task){
    pthread_mutex_lock(&pool->lock);

    if(is_full(pool->task_queue)){
        pthread_mutex_unlock(&pool->lock);
        printf("task_queue is full\n");
        return -1;
    }

    push(pool->task_queue,(long)task);

    pthread_mutex_unlock(&pool->lock);
    sem_post(&pool->m_sem);
    return 1;
}










