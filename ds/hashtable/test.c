#include "./hashtable.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int cmp(HNode* h1,HNode* h2){
    if(h1->hcode == h2->hcode && strcmp(h1->key, h2->key) == 0) return 1;
    return 0;
}

int main(){
    HMap* hmap = NULL;
    hm_init(&hmap);

    HNode* hnode = NULL;

    for(int i=0;i<100000;i++){
        hnode = (HNode*)malloc(sizeof(HNode));
        hnode->next = NULL;
        hnode->key = (char *)malloc(15 * sizeof(char));
        sprintf(hnode->key, "key%d", i);
        hnode->hcode = str_hash(hnode->key, strlen(hnode->key));
        hm_insert(hmap,hnode);
    }

    

    for(int i=0;i<100000;i++){
        hnode = (HNode*)malloc(sizeof(HNode));
        hnode->next = NULL;
        hnode->key = (char *)malloc(15 * sizeof(char));
        sprintf(hnode->key, "key%d", i);
        hnode->hcode = str_hash(hnode->key, strlen(hnode->key));

        HNode* ret = hm_lookup(hmap, hnode, cmp);
        printf("lookup %s ,get %s\n", hnode->key, ret->key);

        HNode* res = hm_pop(hmap, hnode,cmp);
        printf("pop %s\n", res->key);

        ret = hm_lookup(hmap, hnode, cmp);
        if(ret != NULL) printf("2nd lookup %s ,get %s\n", hnode->key, ret->key);
    }


    hm_destroy(hmap);

}