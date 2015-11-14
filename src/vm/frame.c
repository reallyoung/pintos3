#include "vm/frame.h"
#include "threads/vaddr.h"
#include "threads/loader.h"
#include <stdio.h>
#include "threads/malloc.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "vm/swap.h"

void init_frame_table()
{
    list_init(&ft);
    lock_init(&ft_lock);
    //swap_init();
}

void* falloc(enum palloc_flags flags, struct spte* spte)
{
    void* kpage;
RETRY:
    if(flags & PAL_USER == 0)
    {
        //PANIC("falloc not pal_user\n");
        return NULL;
    }
    kpage = palloc_get_page(flags);
    
    if(kpage == NULL)
    {
        //PANIC("palloc fail in falloc\n");
        //evict and retry
        frame_evict();
        lock_release(&ft_lock);
        goto RETRY;
    }
    else
    {
        frame_insert(kpage,spte);
    }
    return kpage;

}
void frame_free(void* faddr)
{
    struct list_elem* e;
    struct frame* f;
    bool find = false;
    lock_acquire(&ft_lock);
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
        pagedir_clear_page(f->t->pagedir, f->spte->vaddr);
        palloc_free_page(faddr);
        free(f);
    }
    else
    {
        PANIC("frame_free failed (not found)\n");
    }
    lock_release(&ft_lock);

}
void frame_insert(void* faddr, struct spte* spte)
{
    struct frame* f= (struct frame*)malloc(sizeof(struct frame));
    f->faddr = faddr;
    f->t = thread_current();
    f->spte = spte;
    //f->pte = lookup_page(f->t->pagedir,);
    lock_acquire(&ft_lock);
    list_push_back(&ft,&f->elem);
    lock_release(&ft_lock);
    
}
void frame_evict()
{
    struct list_elem* e;
    struct frame* f;
    lock_acquire(&ft_lock);
    e = list_begin(&ft);
    while(1)
    {
        f = list_entry(e, struct frame, elem);
        if(pagedir_is_accessed(f->t->pagedir, f->spte->vaddr))
        {//give second chance
            pagedir_set_accessed(f->t->pagedir, f->spte->vaddr, false);
        }
        else
        {//evict this page
            if(f->spte->type == stack_t||pagedir_is_dirty(f->t->pagedir, f->spte->vaddr))
            {// swap out
                f->spte->in_swap = true;
                f->spte->swap_idx = swap_out(f->faddr);
               // pagedir_set_dirty(f->t->pagedir, f->spte->vaddr, false);
            }
            //remove page
            list_remove(&f->elem);
            pagedir_clear_page(f->t->pagedir, f->spte->vaddr);
            palloc_free_page(f->faddr);
            free(f);
            //lock_release(&ft_lock);
            //frame_free(f->faddr);
            //lock_release(&ft_lock);
            break;
        }
        e = list_next(e);
        if( e == list_end(&ft) )
            e = list_begin(&ft);
    }

}
