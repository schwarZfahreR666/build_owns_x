#include <assert.h>
#include <stdlib.h>
#include "hashtable.h"


// n must be a power of 2
static void h_init(HTab **htab, size_t n) {
    assert(n > 0 && ((n - 1) & n) == 0);
    (*htab)->tab = (HNode **)calloc(sizeof(HNode *), n);
    (*htab)->mask = n - 1;
    (*htab)->size = 0;
}

uint64_t str_hash(const char *data, size_t len) {
    uint32_t h = 0x811C9DC5;
    for (size_t i = 0; i < len; i++) {
        h = (h + data[i]) * 0x01000193;
    }
    return h;
}

// hashtable insertion
static void h_insert(HTab *htab, HNode *node) {
    size_t pos = node->hcode & htab->mask;
    HNode *next = htab->tab[pos];
    node->next = next;
    htab->tab[pos] = node;
    htab->size++;
}

// hashtable look up subroutine.
// Pay attention to the return value. It returns the address of
// the parent pointer that owns the target node,
// which can be used to delete the target node.
static HNode **h_lookup(
    HTab *htab, HNode *key, int (*cmp)(HNode *, HNode *))
{
    if (!htab->tab) {
        return NULL;
    }

    size_t pos = key->hcode & htab->mask;
    HNode **from = &htab->tab[pos];
    while (*from) {
        if (cmp(*from, key)) {
            return from;
        }
        from = &(*from)->next;
    }
    return NULL;
}

// remove a node from the chain
static HNode *h_detach(HTab *htab, HNode **from) {
    HNode *node = *from;
    *from = (*from)->next;
    htab->size--;
    return node;
}

void hm_init(HMap** hmap){
    *hmap = (HMap*)malloc(sizeof(HMap));
    (*hmap)->ht1 = (HTab*)malloc(sizeof(HTab));
    (*hmap)->ht2 = (HTab*)malloc(sizeof(HTab));
    (*hmap)->ht1->tab = NULL;
    (*hmap)->ht2->tab = NULL;
    (*hmap)->resizing_pos = 0;
}

static void hm_help_resizing(HMap *hmap) {
    if (hmap->ht2->tab == NULL) {
        return;
    }

    size_t nwork = 0;
    while (nwork < K_RESIZING_WORK && hmap->ht2->size > 0) {
        // scan for nodes from ht2 and move them to ht1
        HNode **from = &hmap->ht2->tab[hmap->resizing_pos];
        if (!*from) {
            hmap->resizing_pos++;
            continue;
        }
        
        h_insert(hmap->ht1, h_detach(hmap->ht2, from));
        nwork++;
    }

    if (hmap->ht2->size == 0) {
        // done
        free(hmap->ht2->tab);
        hmap->ht2->tab = NULL;
    }
}

static void hm_start_resizing(HMap *hmap) {
    assert(hmap->ht2->tab == NULL);
    // create a bigger hashtable and swap them
    hmap->ht2 = hmap->ht1;
    hmap->ht1 = (HTab*)malloc(sizeof(HTab));
    h_init(&hmap->ht1, (hmap->ht2->mask + 1) * 2);
    hmap->resizing_pos = 0;
}



HNode *hm_lookup(
    HMap *hmap, HNode *key, int (*cmp)(HNode *, HNode *))
{
    hm_help_resizing(hmap);
    HNode **from = h_lookup(hmap->ht1, key, cmp);
    if (!from) {
        from = h_lookup(hmap->ht2, key, cmp);
    }
    return from ? *from : NULL;
}


void hm_insert(HMap *hmap, HNode *node) {
    if (hmap->ht1->tab == NULL) {
        h_init(&hmap->ht1, HTAB_INIT_SIZE);
    }
    
    h_insert(hmap->ht1, node);
    
    if (hmap->ht2->tab == NULL) {
        // check whether we need to resize
        size_t load_factor = hmap->ht1->size / (hmap->ht1->mask + 1);
        if (load_factor >= K_MAX_LOAD_FACTOR) {
            hm_start_resizing(hmap);
            
        }
    }
    hm_help_resizing(hmap);
}

HNode *hm_pop(
    HMap *hmap, HNode *key, int (*cmp)(HNode *, HNode *))
{
    hm_help_resizing(hmap);
    HNode **from = h_lookup(hmap->ht1, key, cmp);
    if (from) {
        return h_detach(hmap->ht1, from);
    }
    from = h_lookup(hmap->ht2, key, cmp);
    if (from) {
        return h_detach(hmap->ht2, from);
    }
    return NULL;
}

size_t hm_size(HMap *hmap) {
    return hmap->ht1->size + hmap->ht2->size;
}

void hm_destroy(HMap *hmap) {
    assert(hmap->ht1->size + hmap->ht2->size == 0);
    free(hmap->ht1->tab);
    free(hmap->ht2->tab);
    free(hmap);
}