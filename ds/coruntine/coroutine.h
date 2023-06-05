#ifndef C_COROUTINE_H
#define C_COROUTINE_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

#if __APPLE__ && __MACH__
#include <sys/ucontext.h>
#else
#include <ucontext.h>
#endif

#define STACK_SIZE (1024 * 1024)
#define DEFAULT_COROUTINE 16

#define COROUTINE_DEAD 0
#define COROUTINE_READY 1
#define COROUTINE_RUNNING 2
#define COROUTINE_SUSPEND 3

struct schedule;

typedef void (*coroutine_func)(struct schedule *, void *ud);

struct coroutine;

typedef struct schedule
{
    char stack[STACK_SIZE];
    ucontext_t main;
    int nco;
    int cap;
    int running;
    struct coroutine **co;
} schedule;

typedef struct coroutine
{
    coroutine_func func;
    void *ud;
    ucontext_t ctx;
    struct schedule *sch;
    ptrdiff_t cap;
    ptrdiff_t size;
    int status;
    char *stack;
} coroutine;

coroutine *_co_new(schedule *S, coroutine_func func, void *ud);

schedule *coroutine_open(void);
void coroutine_close(schedule *);

int coroutine_new(schedule *, coroutine_func, void *ud);
void coroutine_resume(schedule *, int id);
int coroutine_status(schedule *, int id);
int coroutine_running(schedule *);
void coroutine_yield(schedule *);

#endif