#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "queue.h"


Queue * create_queue(int size){
    Queue* queue;
    if((queue = (Queue *)malloc(sizeof(Queue))) == NULL){
        return NULL;
    }
    assert(size > 0);
    
    if((queue->queue_array = (QUEUE_TYPE *)malloc(size * sizeof(QUEUE_TYPE))) == NULL){
        free(queue);
        return NULL;
    };
    queue->queue_size = size;
    queue->rear = 0;
    queue->front = -1;
    queue->elements_num = 0;
    return queue;
}

void destroy_queue(Queue * queue){
    while(!is_empty(queue)){
        pop(queue);
    }
    free(queue->queue_array);
    free(queue);
}

int next(Queue* queue,int x){
    return (x + 1) % queue->queue_size;
}


void push(Queue * queue,QUEUE_TYPE value){
    assert(!is_full(queue));
    queue->queue_array[queue->rear] = value;
    queue->rear = next(queue,queue->rear);
    queue->elements_num++;
}

QUEUE_TYPE peek(Queue* queue){
    assert(!is_empty(queue));
    return queue->queue_array[next(queue,queue->front)];
}

QUEUE_TYPE pop(Queue * queue){
    assert(!is_empty(queue));
    QUEUE_TYPE value = peek(queue);
    queue->front = next(queue,queue->front);
    queue->elements_num--;
    return value;
}

int is_empty(Queue* queue){
    return queue->elements_num == 0;
}

int is_full(Queue* queue){
    return queue->elements_num == queue->queue_size;
}
