#include "SPSC_Queue.h"

SPSC_Queue *init()
{
    SPSC_Queue *sq = (SPSC_Queue *)malloc(sizeof(SPSC_Queue));
    memset(sq->blk, 0, QUEUE_SIZE);
    sq->free_write_cnt = BLK_CNT;
    sq->read_idx = 0;
    sq->write_idx = 0;
    return sq;
}

LogBlock *alloc(SPSC_Queue *sq, uint16_t size_)
{
    sq->size = size_ + sizeof(LogBlock);
    uint32_t blk_sz = (sq->size + sizeof(LogBlock) - 1) / sizeof(LogBlock);
    if (blk_sz >= sq->free_write_cnt)
    {
        asm volatile(""
                     : "=m"(sq->read_idx)
                     :
                     :); // force read memory
        uint32_t read_idx_cache = sq->read_idx;
        if (read_idx_cache <= sq->write_idx)
        {
            sq->free_write_cnt = BLK_CNT - sq->write_idx;
            if (blk_sz >= sq->free_write_cnt && read_idx_cache != 0)
            { // wrap around
                sq->blk[0].size = 0;
                asm volatile(""
                             :
                             : "m"(sq->blk)
                             :); // memory fence
                sq->blk[sq->write_idx].size = 1;
                sq->write_idx = 0;
                sq->free_write_cnt = read_idx_cache;
            }
        }
        else
        {
            sq->free_write_cnt = read_idx_cache - sq->write_idx;
        }
        if (sq->free_write_cnt <= blk_sz)
        {
            return NULL;
        }
    }
    return &(sq->blk[sq->write_idx]);
}

void doPush(SPSC_Queue *sq)
{
    uint32_t blk_sz = (sq->size + sizeof(LogBlock) - 1) / sizeof(LogBlock);
    sq->blk[sq->write_idx + blk_sz].size = 0;

    asm volatile(""
                 :
                 : "m"(sq->blk)
                 :); // memory fence
    sq->blk[sq->write_idx].size = sq->size;
    sq->write_idx += blk_sz;
    sq->free_write_cnt -= blk_sz;
}

int push(SPSC_Queue *sq, uint16_t size, LogBlock *data)
{
    LogBlock *logBlock = alloc(sq, size);
    if (!logBlock)
        return -1;
    memcpy(logBlock, data, size);
    doPush(sq);
    return 0;
}

LogBlock *front(SPSC_Queue *sq)
{
    asm volatile(""
                 : "=m"(sq->blk)
                 :
                 :); // force read memory
    uint16_t size = sq->blk[sq->read_idx].size;
    if (size == 1)
    { // wrap around
        sq->read_idx = 0;
        size = sq->blk[0].size;
    }
    if (size == 0)
        return NULL;
    return &(sq->blk[sq->read_idx]);
}

void doPop(SPSC_Queue *sq)
{
    asm volatile(""
                 : "=m"(sq->blk)
                 : "m"(sq->read_idx)
                 :); // memory fence
    uint32_t blk_sz = (sq->blk[sq->read_idx].size + sizeof(LogBlock) - 1) / sizeof(LogBlock);
    sq->read_idx += blk_sz;
    asm volatile(""
                 :
                 : "m"(sq->read_idx)
                 :); // force write memory
}

LogBlock *pop(SPSC_Queue *sq)
{
    LogBlock *logBlock = front(sq);
    if (!logBlock)
        return NULL;
    uint32_t size = logBlock->size - sizeof(LogBlock);
    LogBlock *data = (LogBlock *)malloc(size);
    doPop(sq);
    return data;
}