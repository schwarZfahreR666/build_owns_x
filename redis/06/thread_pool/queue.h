#ifndef QUEUE_TYPE

typedef struct {
    void (*func)(void *arg); //函数指针
    void * arg;           //任务参数
} Task;

#define QUEUE_TYPE int

typedef struct {
    int queue_size;
    //队首元素前一位
    int front;
    //下一个要插入元素的位置
    int rear;
    int elements_num;

    QUEUE_TYPE * queue_array;
}Queue;

void push(Queue * queue,QUEUE_TYPE value);

QUEUE_TYPE pop(Queue * queue);

QUEUE_TYPE peek(Queue * queue);

int is_empty(Queue * queue);

int is_full(Queue * queue);

Queue* create_queue(int size);

void destroy_queue(Queue * queue);


#endif