#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <stdint.h>
#include "threads/synch.h"
#include <list.h>
#include "threads/palloc.h"
#include "vm/page.h"

struct frame{
    void* faddr;
    //unsigned* pte;
    struct spte* spte;
    struct thread* t;
    unsigned vpn;
    struct list_elem elem;
};

struct list ft;
struct lock ft_lock;

void init_frame_table();
void* falloc(enum palloc_flags flags,struct spte* spte);
void frame_free(void* faddr);
void frame_insert(void* faddr, struct spte* spte);
void frame_evict();
void frame_free_on_exit();
void frame_ffree(void* faddr);
#endif
