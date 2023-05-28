#include "skiplist.h"

SkiplistNode *slCreateNode(int level, double key) {
    SkiplistNode *sn = (SkiplistNode *)malloc(sizeof(SkiplistNode)+level*sizeof(struct SkiplistLevel));
    sn->key = key;
    return sn;
}

Skiplist *slCreate(void) {

    Skiplist *sl = (Skiplist *)malloc(sizeof(Skiplist));
    sl->level = 1;
    sl->length = 0;
    sl->header = slCreateNode(SKIPLIST_MAXLEVEL, 0);
    for (int j = 0; j < SKIPLIST_MAXLEVEL; j++) {
        sl->header->level[j].forward = NULL;
        sl->header->level[j].span = 0;
    }
    sl->header->backward = NULL;
    sl->tail = NULL;
    return sl;
}

void slFreeNode(SkiplistNode *node) {
    if(node == NULL) return;

    free(node);
}

void slFree(Skiplist *sl) {
    SkiplistNode *node = sl->header->level[0].forward, *next;

    free(sl->header);
    while(node) {
        next = node->level[0].forward;
        slFreeNode(node);
        node = next;
    }
    free(sl);
}

int slRandomLevel(void) {
    int level = 1;
    while ((random()&0xFFFF) < (SKIPLIST_P * 0xFFFF))
        level += 1;
    return (level<SKIPLIST_MAXLEVEL) ? level : SKIPLIST_MAXLEVEL;
}

SkiplistNode *slInsert(Skiplist *sl, double key) {
    SkiplistNode *update[SKIPLIST_MAXLEVEL], *x;
    unsigned int rank[SKIPLIST_MAXLEVEL];
    int i, level;

    x = sl->header;
    for (i = sl->level-1; i >= 0; i--) {
        /* store rank that is crossed to reach the insert position */
        rank[i] = i == (sl->level-1) ? 0 : rank[i+1];
        while (x->level[i].forward && x->level[i].forward->key < key) {
            rank[i] += x->level[i].span;
            x = x->level[i].forward;
        }
        update[i] = x;
    }
    level = slRandomLevel();
    if (level > sl->level) {
        for (i = sl->level; i < level; i++) {
            rank[i] = 0;
            update[i] = sl->header;
            update[i]->level[i].span = sl->length;
        }
        sl->level = level;
    }
    x = slCreateNode(level,key);
    for (i = 0; i < level; i++) {
        x->level[i].forward = update[i]->level[i].forward;
        update[i]->level[i].forward = x;

        /* update span covered by update[i] as x is inserted here */
        x->level[i].span = update[i]->level[i].span - (rank[0] - rank[i]);
        update[i]->level[i].span = (rank[0] - rank[i]) + 1;
    }

    /* increment span for untouched levels */
    for (i = level; i < sl->level; i++) {
        update[i]->level[i].span++;
    }

    x->backward = (update[0] == sl->header) ? NULL : update[0];
    if (x->level[0].forward)
        x->level[0].forward->backward = x;
    else
        sl->tail = x;
    sl->length++;
    return x;
}


void slDeleteNode(Skiplist *sl, SkiplistNode *x, SkiplistNode **update) {

    for (int i = 0; i < sl->level; i++) {
        if (update[i]->level[i].forward == x) {
            update[i]->level[i].span += x->level[i].span - 1;
            update[i]->level[i].forward = x->level[i].forward;
        } else {
            update[i]->level[i].span -= 1;
        }
    }
    if (x->level[0].forward) {
        x->level[0].forward->backward = x->backward;
    } else {
        sl->tail = x->backward;
    }
    while(sl->level > 1 && sl->header->level[sl->level-1].forward == NULL)
        sl->level--;
    sl->length--;
}

int slDelete(Skiplist *sl, double key) {
    SkiplistNode *update[SKIPLIST_MAXLEVEL], *x;

    x = sl->header;
    for (int i = sl->level-1; i >= 0; i--) {
        while (x->level[i].forward && x->level[i].forward->key < key)
        {
            x = x->level[i].forward;
        }
        update[i] = x;
    }
    
    x = x->level[0].forward;
    if (x && key == x->key) {
        slDeleteNode(sl, x, update);
        slFreeNode(x);
        return 1;
    }
    return 0; /* not found */
}

unsigned long slGetRank(Skiplist *sl, double key) {
    SkiplistNode *x;
    unsigned long rank = 0;

    x = sl->header;
    for (int i = sl->level-1; i >= 0; i--) {
        while (x->level[i].forward && x->level[i].forward->key < key) {
            rank += x->level[i].span;
            x = x->level[i].forward;
        }

        return rank;
    }
    return 0;
}

SkiplistNode* slGetElementByRank(Skiplist *zsl, unsigned long rank) {
    SkiplistNode *x;
    unsigned long traversed = 0;

    x = zsl->header;
    for (int i = zsl->level-1; i >= 0; i--) {
        while (x->level[i].forward && (traversed + x->level[i].span) <= rank)
        {
            traversed += x->level[i].span;
            x = x->level[i].forward;
        }
        if (traversed == rank) {
            return x;
        }
    }
    return NULL;
}