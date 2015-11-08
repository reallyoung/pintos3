#include "vm/frame.h"
#include "threads/vaddr.h"
#include "threads/loader.h"
#include <stdio.h>
#include "threads/malloc.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"

void init_frame_table()
{
    list_init(&ft);
    lock_init(&ft_lock);
}

void* falloc(enum palloc_flags flags)
{
    void* kpage;
RETRY:
    kpage = palloc_get_page(flags);
    
    if(kpage == NULL)
    {
        PANIC("palloc fail in falloc\n");
        //evict and retry
        frame_evict();
        goto RETRY;
    }
    else
    {
        frame_insert(kpage);
    }
    return kpage;

}
void frame_free(void* faddr)
{
    struct list_elem* e;
    struct frame* f;
    bool find = false;
    for(e=list_begin(&ft);e!=list_end(&ft);e=list_next(e))
    {
        f = list_entry(e, struct frame, elem);
        if(f->faddr == faddr)
        {
            find = true;
            break;
        }
    }
    if(find)
    {
        list_remove(&f->elem);
        palloc_free_page(faddr);
    }
    else
    {
        PANIC("frame_free failed (not found)\n");
    }

}
void frame_insert(void* faddr)
{
    struct frame* f= (struct frame*)malloc(sizeof(struct frame));
    f->faddr = faddr;
    f->t = thread_current();
    //f->pte = lookup_page(f->t->pagedir,);
    list_push_back(&ft,&f->elem);
    
}
void frame_evict()
{
    return;
}
