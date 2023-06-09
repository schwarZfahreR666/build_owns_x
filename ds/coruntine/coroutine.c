#include "coroutine.h"

coroutine *_co_new(schedule *S, coroutine_func func, void *ud)
{
    coroutine *co = (coroutine *)malloc(sizeof(coroutine));
    co->func = func;
    co->ud = ud;
    co->sch = S;
    co->cap = 0;
    co->size = 0;
    co->status = COROUTINE_READY;
    co->stack = NULL;
    return co;
}

void _co_delete(coroutine *co)
{
    free(co->stack);
    free(co);
}

schedule *
coroutine_open(void)
{
    schedule *S = malloc(sizeof(*S));
    S->nco = 0;
    S->cap = DEFAULT_COROUTINE;
    S->running = -1;
    S->co = malloc(sizeof(coroutine *) * S->cap);
    memset(S->co, 0, sizeof(coroutine *) * S->cap);
    return S;
}

void coroutine_close(schedule *S)
{
    int i;
    for (i = 0; i < S->cap; i++)
    {
        coroutine *co = S->co[i];
        if (co)
        {
            _co_delete(co);
        }
    }
    free(S->co);
    S->co = NULL;
    free(S);
}

int coroutine_new(schedule *S, coroutine_func func, void *ud)
{
    coroutine *co = _co_new(S, func, ud);
    if (S->nco >= S->cap)
    {
        int id = S->cap;
        S->co = realloc(S->co, S->cap * 2 * sizeof(coroutine *));
        memset(S->co + S->cap, 0, sizeof(coroutine *) * S->cap);
        S->co[S->cap] = co;
        S->cap *= 2;
        ++S->nco;
        return id;
    }
    else
    {
        int i;
        for (i = 0; i < S->cap; i++)
        {
            int id = (i + S->nco) % S->cap;
            if (S->co[id] == NULL)
            {
                S->co[id] = co;
                ++S->nco;
                return id;
            }
        }
    }
    assert(0);
    return -1;
}

static void
mainfunc(uint32_t low32, uint32_t hi32)
{
    uintptr_t ptr = (uintptr_t)low32 | ((uintptr_t)hi32 << 32);
    schedule *S = (schedule *)ptr;
    int id = S->running;
    coroutine *C = S->co[id];
    C->func(S, C->ud);
    _co_delete(C);
    S->co[id] = NULL;
    --S->nco;
    S->running = -1;
}

void coroutine_resume(schedule *S, int id)
{
    assert(S->running == -1);
    assert(id >= 0 && id < S->cap);
    coroutine *C = S->co[id];
    if (C == NULL)
        return;
    int status = C->status;
    switch (status)
    {
    case COROUTINE_READY:
        getcontext(&C->ctx);
        C->ctx.uc_stack.ss_sp = S->stack;
        C->ctx.uc_stack.ss_size = STACK_SIZE;
        C->ctx.uc_link = &S->main;
        S->running = id;
        C->status = COROUTINE_RUNNING;
        uintptr_t ptr = (uintptr_t)S;
        makecontext(&C->ctx, (void (*)(void))mainfunc, 2, (uint32_t)ptr, (uint32_t)(ptr >> 32));
        swapcontext(&S->main, &C->ctx);
        break;
    case COROUTINE_SUSPEND:
        memcpy(S->stack + STACK_SIZE - C->size, C->stack, C->size);
        S->running = id;
        C->status = COROUTINE_RUNNING;
        swapcontext(&S->main, &C->ctx);
        break;
    default:
        assert(0);
    }
}

static void
_save_stack(coroutine *C, char *top)
{
    char dummy = 0;
    assert(top - &dummy <= STACK_SIZE);
    if (C->cap < top - &dummy)
    {
        free(C->stack);
        C->cap = top - &dummy;
        C->stack = malloc(C->cap);
    }
    C->size = top - &dummy;
    memcpy(C->stack, &dummy, C->size);
}

void coroutine_yield(schedule *S)
{
    int id = S->running;
    assert(id >= 0);
    coroutine *C = S->co[id];
    assert((char *)&C > S->stack);
    _save_stack(C, S->stack + STACK_SIZE);
    C->status = COROUTINE_SUSPEND;
    S->running = -1;
    swapcontext(&C->ctx, &S->main);
}

int coroutine_status(schedule *S, int id)
{
    assert(id >= 0 && id < S->cap);
    if (S->co[id] == NULL)
    {
        return COROUTINE_DEAD;
    }
    return S->co[id]->status;
}

int coroutine_running(schedule *S)
{
    return S->running;
}