#ifndef SKIP_LIST
#define SKIP_LIST

#include <stdlib.h>

#define SKIPLIST_MAXLEVEL 16
#define SKIPLIST_P 0.25

typedef struct SkiplistNode {
    double key;
    struct SkiplistNode *backward;
    struct SkiplistLevel {
        struct SkiplistNode *forward;
        unsigned int span;
    } level[];
} SkiplistNode;

typedef struct Skiplist {
    SkiplistNode *header, *tail;
    unsigned long length;
    int level;
} Skiplist;


#endif // SKIP_LIST