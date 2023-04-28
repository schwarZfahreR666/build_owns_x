#include <stdio.h>
#include <stdlib.h>

#include "queue.h"

int nnext(Queue* queue,int x){
    return (x + 1) % queue->queue_size;
}

int main(){
    int input[10] = {1,3,5,7,9,10,8,6,4,2};
    Queue* queue = create_queue(10);
    for(int i=0;i<5;i++){
        printf("%d ",input[i]);
        push(queue,input[i]);
    }
    printf("\n");
    while(!is_empty(queue)){
        // printf("%d ",peek());
        printf("element:%d ",pop(queue));
        printf("front:%d rear:%d next:%d\n",queue->front,queue->rear,nnext(queue,queue->front));
    }

    for(int i=5;i<10;i++){
        printf("%d ",input[i]);
        push(queue,input[i]);
    }
    printf("\n");
    while(!is_empty(queue)){
        // printf("%d ",peek());
        printf("element:%d ",pop(queue));
        printf("front:%d rear:%d next:%d\n",queue->front,queue->rear,nnext(queue,queue->front));
    }
    destroy_queue(queue);

    return 0;
}