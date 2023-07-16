#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define QUEUE_SIZE 1024 * 1024 * 8

typedef struct LogBlock
{
    uint16_t size;
    uint16_t logType;
    uint16_t groupId;
} LogBlock;

#define BLK_CNT QUEUE_SIZE / sizeof(LogBlock)

typedef struct SPSC_Queue
{
    LogBlock blk[BLK_CNT] __attribute__((aligned (64)));

    uint32_t write_idx __attribute__((aligned (128)));
    uint32_t free_write_cnt;
    uint16_t size;

    uint32_t read_idx __attribute__((aligned (128)));
} SPSC_Queue;