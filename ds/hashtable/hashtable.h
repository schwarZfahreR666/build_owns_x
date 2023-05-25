#include <stddef.h>
#include <stdint.h>

#define K_RESIZING_WORK 128

#define K_MAX_LOAD_FACTOR 8

#define HTAB_INIT_SIZE 4

// hashtable node, should be embedded into the payload
typedef struct HNODE {
    struct HNODE *next;
    uint64_t hcode;
} HNode;

// a simple fixed-sized hashtable
typedef struct HTab {
    HNode **tab;
    size_t mask;
    size_t size;
} HTab;

// the real hashtable interface.
// it uses 2 hashtables for progressive resizing.
typedef struct HMap {
    HTab* ht1;
    HTab* ht2;
    size_t resizing_pos;
} HMap;

uint64_t str_hash(const uint8_t *data, size_t len);
HNode *hm_lookup(HMap *hmap, HNode *key, int (*cmp)(HNode *, HNode *));
void hm_insert(HMap *hmap, HNode *node);
HNode *hm_pop(HMap *hmap, HNode *key, int (*cmp)(HNode *, HNode *));
size_t hm_size(HMap *hmap);
void hm_destroy(HMap *hmap);
