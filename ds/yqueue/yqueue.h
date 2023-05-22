#ifndef __Y_QUEUE_H__
#define __Y_QUEUE_H__

#include <stdlib.h>
#include <stdatomic.h>
#include <stdio.h>

#define true 1
#define false 0
#define ssize_t unsigned int
#define x_chunk_ptr_t x_chunk_t *
#define x_atomic_ptr_t _Atomic(unsigned long)
#define x_atomic_size_t _Atomic(unsigned int)

#define _EN 8
typedef struct YNode{
    int id;
} YNode;

typedef struct x_chunk_t
    {
        YNode ** xet_array;   ///< 当前内存块中的元素节点数组
        x_chunk_t   * xprev_ptr;   ///< 指向前一内存块节点
        x_chunk_t   * xnext_ptr;   ///< 指向后一内存块节点
    } x_chunk_t;


int yqueue_init();
int yqueue_free(void);
x_chunk_ptr_t alloc_chunk(void);
void free_chunk(x_chunk_ptr_t xchunk_ptr);
void move_begin_pos(void);
void move_end_pos(void);
size_t size(void);
int empty(void);
YNode *back(void);
YNode *front(void);
void push(YNode *xemt_value);
void pop(void);


#endif // __Y_QUEUE_H__