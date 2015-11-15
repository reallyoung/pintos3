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
    if(flags & PAL_USER == 0)
    {
        //PANIC("falloc not pal_user\n");
        return NULL;
    }
   // printf("0 %s %d\n",spte->t->name, spte->t->tid);
    lock_acquire(&ft_lock);
RETRY:
   // printf("1 %s %d\n",spte->t->name, spte->t->tid);
    kpage = palloc_get_page(flags);
    if(kpage == NULL)
    {
     //   printf("2 %s %d\n",spte->t->name, spte->t->tid);
        frame_evict();
      //  printf("3 %s %d\n",spte->t->name, spte->t->tid);
        goto RETRY;
    }
    else
    {
       // printf("4 %s %d\n",spte->t->name, spte->t->tid);
        frame_insert(kpage,spte);
       // printf("5 %s %d\n",spte->t->name, spte->t->tid);
    }
    lock_release(&ft_lock);
  //  printf("6 %s %d\n",spte->t->name, spte->t->tid);
    return kpage;

}
void frame_free(void* faddr)
{
    struct list_elem* e;
    struct frame* f;
    bool find = false;
    //printf("10\n");
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
    //printf("11\n");
    lock_release(&ft_lock);
    //printf("12\n");

}
void frame_insert(void* faddr, struct spte* spte)
{
    struct frame* f= (struct frame*)malloc(sizeof(struct frame));
    f->faddr = faddr;
    f->t = thread_current();
    f->spte = spte;
    //f->pte = lookup_page(f->t->pagedir,);
    //lock_acquire(&ft_lock);
    list_push_back(&ft,&f->elem);
    //lock_release(&ft_lock);
    
}
void frame_evict()
{
    struct list_elem* e;
    struct frame* f;
    //lock_acquire(&ft_lock);
    e = list_begin(&ft);
    while(1)
    {
        f = list_entry(e, struct frame, elem);
        if(f->spte->pinned)
            goto NXT;
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
NXT:
        e = list_next(e);
        if( e == list_end(&ft) )
            e = list_begin(&ft);
    }

}
void frame_free_on_exit()
{
    struct thread* t = thread_current();
    struct list_elem* e;
    struct list_elem* ne;
    struct frame* f;
    e = list_begin(&ft);
    //ne = e;
   //printf("01\n");
    while(e != list_end(&ft))
    {
        ne = list_next(e);
        f = list_entry(e,struct frame, elem);
        if(f->t == t)
        {
            if(f->t->pagedir == NULL)
                PANIC("pagedir is null\n");
            list_remove(e);
            pagedir_clear_page(f->t->pagedir, f->spte->vaddr);
            palloc_free_page(f->faddr);
            free(f);
        }
        e = ne;
    }
    //printf("02\n");
}
