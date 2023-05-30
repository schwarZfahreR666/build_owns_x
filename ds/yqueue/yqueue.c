#include "yqueue.h"

x_chunk_ptr_t m_chk_begin_ptr = NULL; ///< 内存块链表的起始块
ssize_t m_xst_begin_pos = 0;          ///< 队列中的首个元素位置
x_chunk_ptr_t m_chk_end_ptr = NULL;   ///< 内存块链表的结束块
ssize_t m_xst_end_pos = 0;            ///< 队列中的元素结束位置
x_chunk_ptr_t m_chk_back_ptr = NULL;  ///< 内存块链表的结尾块
ssize_t m_xst_back_pos = 0;           ///< 队列中的结尾元素位置
x_atomic_size_t m_xst_queue_size = 0; ///< 队列中的有效元素数量
x_atomic_ptr_t m_chk_swap_ptr = 0;    ///< 用于保存临时内存块（备用缓存块）

int yqueue_init()
{
    atomic_init(&m_xst_queue_size, 0);
    atomic_init(&m_chk_swap_ptr, 0);

    m_chk_begin_ptr = m_chk_end_ptr = alloc_chunk();

    return 0;
}

int yqueue_free(void)
{
    x_chunk_t *xchunk_ptr = NULL;

    while (size() > 0)
        pop();

    while (true)
    {
        if (m_chk_begin_ptr == m_chk_end_ptr)
        {
            free_chunk(m_chk_begin_ptr);
            break;
        }

        xchunk_ptr = m_chk_begin_ptr;
        m_chk_begin_ptr = m_chk_begin_ptr->xnext_ptr;
        if (xchunk_ptr != NULL)
            free_chunk(xchunk_ptr);
    }

    xchunk_ptr = (x_chunk_ptr_t)atomic_exchange(&m_chk_swap_ptr, (unsigned long)NULL);
    if (xchunk_ptr != NULL)
        free_chunk(xchunk_ptr);
}

/**
 *申请一个存储元素节点的内存块。
 */
x_chunk_ptr_t alloc_chunk(void)
{

    x_chunk_ptr_t xchunk_ptr = (x_chunk_ptr_t)malloc(sizeof(x_chunk_t));
    if (xchunk_ptr == NULL)
    {
        printf("chunk memory allocate failed\n");
        return NULL;
    }

    xchunk_ptr->xet_array = (YNode **)malloc(sizeof(YNode *) * _EN);

    if (xchunk_ptr->xet_array == NULL)
    {
        printf("YNode array memory allocate failed\n");
        return NULL;
    }

    xchunk_ptr->xprev_ptr = NULL;
    xchunk_ptr->xnext_ptr = NULL;

    return xchunk_ptr;
}

/**
 * 释放一个存储元素节点的内存块。
 */
void free_chunk(x_chunk_ptr_t xchunk_ptr)
{
    if (xchunk_ptr != NULL)
    {
        if (xchunk_ptr->xet_array != NULL)
            free(xchunk_ptr->xet_array);

        free(xchunk_ptr);
    }
}

/**********************************************************/
/**
 * 将起始端位置向后移（该接口仅由 pop() 接口调用）。
 */
void move_begin_pos(void)
{
    if (++m_xst_begin_pos == _EN)
    {
        x_chunk_ptr_t xchunk_ptr = m_chk_begin_ptr;
        m_chk_begin_ptr = m_chk_begin_ptr->xnext_ptr;

        if (m_chk_begin_ptr == NULL)
        {
            printf("no available next chunk\n");
            return;
        }

        m_chk_begin_ptr->xprev_ptr = NULL;
        m_xst_begin_pos = 0;

        xchunk_ptr = (x_chunk_ptr_t)atomic_exchange(&m_chk_swap_ptr, (unsigned long)xchunk_ptr);
        if (xchunk_ptr != NULL)
            free_chunk(xchunk_ptr);
    }
}

/**
 * 将结束端位置向后移（该接口仅由 push() 接口调用）。
 */
void move_end_pos(void)
{
    if (++m_xst_end_pos == _EN)
    {
        x_chunk_ptr_t xchunk_ptr = (x_chunk_ptr_t)atomic_exchange(&m_chk_swap_ptr, (unsigned long)NULL);
        if (xchunk_ptr != NULL)
        {
            m_chk_end_ptr->xnext_ptr = xchunk_ptr;
            xchunk_ptr->xprev_ptr = m_chk_end_ptr;
        }
        else
        {
            m_chk_end_ptr->xnext_ptr = alloc_chunk();
            m_chk_end_ptr->xnext_ptr->xprev_ptr = m_chk_end_ptr;
        }

        m_chk_end_ptr = m_chk_end_ptr->xnext_ptr;
        m_xst_end_pos = 0;
    }
}

size_t size(void)
{
    return atomic_load(&m_xst_queue_size);
}

/**
 * 判断队列是否为空。
 */
int empty(void)
{
    return (0 == size());
}

/**
 * 返回队列末端元素。
 */
YNode *back(void)
{
    if (empty() == true)
    {
        printf("no elements in yqueue\n");
        return NULL;
    }
    return m_chk_back_ptr->xet_array[m_xst_back_pos];
}

/**
 * 返回队列首个元素。
 */
YNode *front(void)
{
    if (empty() == true)
    {
        printf("no elements in yqueue\n");
        return NULL;
    }
    return m_chk_begin_ptr->xet_array[m_xst_begin_pos];
}

/**
 * 向队列尾端压入一个元素。
 */
void push(YNode *xemt_value)
{
    m_chk_end_ptr->xet_array[m_xst_end_pos] = xemt_value;

    m_chk_back_ptr = m_chk_end_ptr;
    m_xst_back_pos = m_xst_end_pos;

    move_end_pos();
    atomic_fetch_add(&m_xst_queue_size, 1);
}

/**
 * 从队列前端弹出一个元素。
 */
void pop(void)
{
    if (empty())
        return;
    atomic_fetch_sub(&m_xst_queue_size, 1);
    free(m_chk_begin_ptr->xet_array[m_xst_begin_pos]);
    move_begin_pos();
}