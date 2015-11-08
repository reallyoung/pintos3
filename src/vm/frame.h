#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <stdint.h>
#include "threads/synch.h"
#include <list.h>
#include "threads/palloc.h"
struct frame{
    void* faddr;
    unsigned* pte;//spte
    struct thread* t;
    unsigned vpn;
    struct list_elem elem;
};

struct list ft;
struct lock ft_lock;

void init_frame_table();
void* falloc(enum palloc_flags flags);
void frame_free(void* faddr);
void frame_insert(void* faddr);
void frame_evict();

#endif
