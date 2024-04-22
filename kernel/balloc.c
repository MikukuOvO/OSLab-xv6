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
    int siz;
    bool is_used;
};

#define HEAD_SIZE sizeof(struct header)

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
    h->is_used = false;
}

void*
bmemalloc(uint64 nbytes){
    if(nbytes == 0) return 0;
    acquire(&bmem.lock);
    struct header *pos = 0;
    uint64 mindelta = BALLOC_OFFSET;
    for(struct header *cur = bmem.freelist; cur; cur = cur->nxt){
        if(cur->is_used == false && cur->siz >= nbytes){
            uint64 curdelta = cur->siz - nbytes;
            if(curdelta < mindelta){
                mindelta = curdelta;
                pos = cur;
            }
        }
    }
    if(pos>0){
        pos->is_used = true;
        uint64 res = pos->siz - nbytes - HEAD_SIZE;
        if(res > 0){ // 剩余数据块部分大于 HEAD_SIZE 的大小，能够再分裂出一个新数据块
            struct header *newpos = (struct header*)((uint64)pos + nbytes + HEAD_SIZE);
            newpos->pre = pos;
            newpos->nxt = pos->nxt;
            newpos->siz = res;
            newpos->is_used = false;
            if(pos->nxt) pos->nxt->pre = newpos;
            pos->nxt = newpos;
        }
        pos->siz = nbytes;
        pos = (struct header *)((uint64)pos + HEAD_SIZE);
        memset(pos, 5, nbytes); // fill with junk
    }
    release(&bmem.lock);
    return pos;
}

void
bmemfree(void *ad){
    struct header *pos = (struct header *)((uint64)ad - HEAD_SIZE);
    pos->is_used = false;
    struct header *newpos = pos->nxt;
    if(newpos && newpos->is_used == false){
        if(newpos->nxt) newpos->nxt->pre = pos;
        pos->nxt = newpos->nxt;
        pos->siz = pos->siz + newpos->siz + HEAD_SIZE;
    }
    newpos = pos->pre;
    if(newpos && newpos->is_used == false){
        if(pos->nxt) pos->nxt->pre = newpos;
        newpos->nxt = pos->nxt;
        newpos->siz = newpos->siz + pos->siz + HEAD_SIZE;
    }
}

void
showblock(){
    acquire(&bmem.lock);
    printf("Start Print\n");
    struct header *cur = bmem.freelist;
    while(cur){
        printf("Block Size: %d, Is_used: %d\n", cur->siz, cur->is_used);
        cur = cur->nxt;
    }
    release(&bmem.lock);
}

void
bmemprint(){
    static char *parray[5];
    showblock();
    for(int i=0;i<4;++i){
        parray[i] = bmemalloc(i+1);
        showblock();
    }
    for(int i=0;i<4;++i){
        bmemfree(parray[i]);
        showblock();
    }
}