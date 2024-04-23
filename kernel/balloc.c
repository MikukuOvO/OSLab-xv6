//
// Created by Fenglin Yu on 2024/4/22.
//
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "stdbool.h"

struct header{
    struct header *pre, *nxt;
    int siz, data_siz; // 防止内存泄漏
    int is_used;
};

#define HEAD_SIZE sizeof(struct header) // 32，64位8对齐方式

struct {
    struct spinlock lock;
    struct header *freelist;
}bmem;

extern char end[];

void
bmeminit(){
    initlock(&bmem.lock, "bmem");
    bmem.freelist = (struct header*)BALLOC_START;
    struct header *h = bmem.freelist;
    h->pre = h->nxt = 0;
    h->siz = BALLOC_OFFSET - HEAD_SIZE;
    h->data_siz = 0;
    h->is_used = 0;
}

void*
bmemalloc(uint64 nbytes){
    if(nbytes == 0) return 0;
    acquire(&bmem.lock);
    struct header *pos = 0;
    uint64 mindelta = BALLOC_OFFSET;
    for(struct header *cur = bmem.freelist; cur; cur = cur->nxt){
        if(cur->is_used == 0 && cur->siz >= nbytes){
            uint64 curdelta = cur->siz - nbytes;
            if(curdelta < mindelta){
                mindelta = curdelta;
                // printf("curdelta=%d\n", curdelta);
                pos = cur;
            }
        }
    }
    if(pos>0){
        pos->is_used = 1;
        int res = pos->siz - nbytes - HEAD_SIZE;
        if(res > 0){ // 剩余数据块部分大于 HEAD_SIZE 的大小，能够再分裂出一个新数据块
            struct header *newpos = (struct header*)((uint64)pos + nbytes + HEAD_SIZE);
            newpos->pre = pos;
            newpos->nxt = pos->nxt;
            newpos->siz = res;
            newpos->is_used = 0;
            newpos->data_siz = 0;
            if(pos->nxt) pos->nxt->pre = newpos;
            pos->nxt = newpos;
            pos->data_siz = nbytes;
            pos->siz = nbytes;
        }
        else pos->data_siz = nbytes;
        pos = (struct header *)((uint64)pos + HEAD_SIZE);
        memset(pos, 5, nbytes); // fill with junk
    }
    release(&bmem.lock);
    return pos;
}

void
bmemfree(void *ad){
    struct header *pos = (struct header *)((uint64)ad - HEAD_SIZE);
    pos->is_used = 0;
    pos->data_siz = 0;
    struct header *newpos = pos->nxt;
    if(newpos && newpos->is_used == 0){
        if(newpos->nxt) newpos->nxt->pre = pos;
        pos->nxt = newpos->nxt;
        pos->siz += newpos->siz + HEAD_SIZE;
    }
    newpos = pos->pre;
    if(newpos && newpos->is_used == 0){
        if(pos->nxt) pos->nxt->pre = newpos;
        newpos->nxt = pos->nxt;
        newpos->siz += pos->siz + HEAD_SIZE;
    }
}

void
showblock(){
    acquire(&bmem.lock);
    struct header *cur = bmem.freelist;
    while(cur){
        printf("Block Size: %d, Is_used: %d, Data Size: %d\n", cur->siz, cur->is_used, cur->data_siz);
        cur = cur->nxt;
    }
    release(&bmem.lock);
}

// Test Cases
void
bmemprint(){
    printf("HEAD_SIZE=%d\n",HEAD_SIZE);
    showblock();
    printf("--------------------Test 1--------------------\n");
    printf("Test for basic memory allocation\n");
    static char *parray[5];
    for(int i=0;i<4;++i){
        printf("~~~~~~~~~~~~~~~~~~Allocate %d bytes~~~~~~~~~~~~~~~~~~\n", i+1);
        parray[i] = bmemalloc(i+1);
        showblock();
    }
    for(int i=0;i<4;++i){
        printf("~~~~~~~~~~~~~~~~~~Free %d bytes~~~~~~~~~~~~~~~~~~\n", i+1);
        bmemfree(parray[i]);
        showblock();
    }

    printf("--------------------Test 2--------------------\n");
    printf("Test for memory allocate strategy\n");
    static int c[4] = {3, 1, 2, 5};
    for(int i=0;i<4;++i){
        printf("~~~~~~~~~~~~~~~~~~Allocate %d bytes~~~~~~~~~~~~~~~~~~\n", c[i]);
        parray[i] = bmemalloc(c[i]);
        showblock();
    }
    printf("~~~~~~~~~~~~~~~~~~Free %d bytes~~~~~~~~~~~~~~~~~~\n", c[0]);
    bmemfree(parray[0]);
    showblock();
    printf("~~~~~~~~~~~~~~~~~~Free %d bytes~~~~~~~~~~~~~~~~~~\n", c[2]);
    bmemfree(parray[2]);
    showblock();
    printf("~~~~~~~~~~~~~~~~~~Allocate %d bytes~~~~~~~~~~~~~~~~~~\n", c[1]);
    parray[4] = bmemalloc(c[1]);
    showblock();
    printf("~~~~~~~~~~~~~~~~~~Free %d bytes~~~~~~~~~~~~~~~~~~\n", c[2]);
    bmemfree(parray[3]);
    showblock();
    printf("~~~~~~~~~~~~~~~~~~Free %d bytes~~~~~~~~~~~~~~~~~~\n", c[2]);
    bmemfree(parray[4]);
    showblock();
    printf("~~~~~~~~~~~~~~~~~~Free %d bytes~~~~~~~~~~~~~~~~~~\n", c[2]);
    bmemfree(parray[1]);
    showblock();
    /*
    */
}