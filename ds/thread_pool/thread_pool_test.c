#include "thread_pool.h"


void task(void* arg) {
    int* num = (int*)arg;
    printf("Task #%d is running on thread %d \n", *num, (int)pthread_self());
    free(num);
    return;
}

int main() {
    ThreadPool* pool;
    pool = threadpool_create(4,10);

    for (int i = 1; i <= 100; i++) {
        int* num = (int*)malloc(sizeof(int));
        *num = i;
        Task* task_s = (Task*)malloc(sizeof(Task));
        task_s->func = (void (*)(void *arg))malloc(sizeof(void (*)(void *arg)));
        task_s->func = task;
        task_s->arg = (int*)malloc(sizeof(int));
        task_s->arg = num;
        printf("append task %d\n",i);
        append_task(pool, task_s);
    }

    printf("***********************\n");
    for (int i = 11; i <= 200; i++) {
        int* num = (int*)malloc(sizeof(int));
        *num = i;
        Task* task_s = (Task*)malloc(sizeof(Task));
        task_s->func = (void (*)(void *arg))malloc(sizeof(void (*)(void *arg)));
        task_s->func = task;
        task_s->arg = (int*)malloc(sizeof(int));
        task_s->arg = num;
        printf("append task %d\n",i);
        append_task(pool, task_s);
    }

    while(1);

    destory_threadpool(pool);
    return 0;
}