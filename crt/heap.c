#include "crt.h"

//内存块结构，连接成双向链表
typedef struct _heap_header{
    enum{
        HEAP_BLOCK_FREE = 0xABABABAB, //magic number of free block
        HEAP_BLOCK_USED = 0xCDCDCDCD,//magic number of used block
    } type;

    unsigned long size;
    struct _heap_header* next;
    struct _heap_header* prev;
} heap_header;

#define ADDR_ADD(a,o) (((char*)(a)) + o)
#define HEADER_SIZE (sizeof(heap_header))

heap_header* list_head = NULL;

void* malloc(unsigned long size){
    heap_header *header;

    if(size == 0) return NULL;
    header = list_head;
    while(header != NULL){
        //找到未使用的第一个块
        if(header->type == HEAP_BLOCK_USED){
            header = header->next;
            continue;
        }
        
        if(header->size > size + HEADER_SIZE &&
        header->size <= size + HEADER_SIZE * 2){
            header->type = HEAP_BLOCK_USED;

            //需测试该句
            // return ADDR_ADD(header,HEADER_SIZE);
        }
        //块太大需要分割
        if(header->size > size + HEADER_SIZE * 2){
            //分出一个size+HEADER_SIZE的块，分配为USED
            heap_header* next = (heap_header*)ADDR_ADD(header,size + HEADER_SIZE);
            next->prev = header;
            next->next = header->next;
            next->type = HEAP_BLOCK_FREE;
            next->size = header->size - (size + HEADER_SIZE);

            header->next = next;
            header->size = size + HEADER_SIZE;
            header->type = HEAP_BLOCK_USED;
            return ADDR_ADD(header,HEADER_SIZE);
        }
        header = header->next;
    }

    return NULL;
}

void free(void *ptr){
    heap_header* header = (heap_header*)ADDR_ADD(ptr,-HEADER_SIZE);
    if(header->type != HEAP_BLOCK_USED) return;

    header->type = HEAP_BLOCK_FREE;
    //前面的块如果也没有使用过，则可以合并
    if(header->prev != NULL && header->prev->type == HEAP_BLOCK_FREE){
        //merge，调整链表和大小
        header->prev->next = header->next;
        if(header->next != NULL) header->next->prev = header->prev;
        header->prev->size += header->size;

        header = header->prev;
    }
    //后面的块如果没有使用，则也可以合并
    if(header->next != NULL && header->next->type == HEAP_BLOCK_FREE){
        //增加大小即可
        header->size += header->next->size;
        header->next = header->next->next;
    }

}

static long brk(void* end_data_segement){
    long ret = 0;
    asm("movq $45,%%rax \n\t"
        "movq %1,%%rbx \n\t"
        "int $0x80 \n\t"
        "movq %%rax,%0 \n\t":
        "=r"(ret):"m"(end_data_segement));

    return ret;
}

long crt_init_heap(){
    void* base = NULL;
    heap_header *header = NULL;
    //32 MB heap size
    unsigned heap_size = 1024 * 1024 * 32;

    //设置一下进程数据段的边界
    base = (void*)brk(0);
    void* end = ADDR_ADD(base,heap_size);

    end = (void*)brk(end);
    if(!end) return 0;

    header = (heap_header*)base;

    header->size = heap_size;
    header->type = HEAP_BLOCK_FREE;
    header->next = NULL;
    header->prev = NULL;

    list_head = header;
    return 1;
}


