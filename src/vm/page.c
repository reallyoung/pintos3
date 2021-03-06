#include "vm/page.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/malloc.h"
#include "vm/frame.h"
#include "vm/swap.h"
#include "userprog/syscall.h"
//#include "threads/"

static unsigned spt_hash_func(const struct hash_elem* e, void *aux UNUSED)
{
    struct spte* s = hash_entry(e, struct spte, elem);
    return hash_int((int)(s->vaddr));
}
static bool spt_less_func(const struct hash_elem *a,
                          const struct hash_elem *b,
                          void *aux UNUSED)
{
    struct spte* sa = hash_entry(a, struct spte, elem);
    struct spte* sb = hash_entry(b, struct spte, elem);
    return sa->vaddr < sb->vaddr;

}
static void spt_action_func(struct hash_elem *e, void *aux UNUSED)
{
    struct spte* s = hash_entry(e, struct spte, elem);
    lock_acquire(&swap_lock);
//    lock_acquire(&ft_lock);
    if(s->in_swap)
        clear_swap(s->swap_idx);
  //  frame_ffree(pagedir_get_page(s->t->pagedir, s->vaddr));
   // lock_release(&ft_lock);
    lock_release(&swap_lock);
    free(s);
}
void free_page_table(struct hash* spt)
{
    hash_destroy(spt,spt_action_func);
}
bool init_spt(struct hash* spt)
{
   return hash_init(spt,spt_hash_func, spt_less_func, NULL);
}
bool
install_page_s (void *upage, void *kpage, bool writable)
{
    struct thread *t = thread_current (); 

    /* Verify that there's not already a page at that virtual
     *      address, then map our page there. */
    if(pagedir_get_page (t->pagedir, upage) != NULL)
        printf("33333\n\n");

   // printf("44444\n\n");
    return (pagedir_get_page (t->pagedir, upage) == NULL
            && pagedir_set_page (t->pagedir, upage, kpage, writable));
}
struct spte* get_spte(struct hash* spt, void* vaddr)
{
    struct hash_elem* e;
    struct spte ispte;
    unsigned mask = 0xfffff000;
    ispte.vaddr = (void *)((unsigned)vaddr & mask);

    e = hash_find(spt,&ispte.elem);
    if(e == NULL)
        return NULL;

    return (struct spte*)hash_entry(e,struct spte, elem);
}
bool load_from_file(struct spte* s)
{
    struct file* file = s->file;
    off_t ofs = s->offset;
    void *upage = s->vaddr;
    uint32_t read_bytes = s->read_byte;
    uint32_t zero_bytes = s->zero_byte;
    bool writable = s->writable;

    ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0); 
    ASSERT (pg_ofs (upage) == 0); 
    //ASSERT (ofs % PGSIZE == 0); 
    //if(ofs % PGSIZE != 0)
    //    sys_exit(-1);
    file_seek (file, ofs);
    if (read_bytes > 0 || zero_bytes > 0)  
    {   
        /* Calculate how to fill this page.
         *          We will read PAGE_READ_BYTES bytes from FILE
         *                   and zero the final PAGE_ZERO_BYTES bytes. */
        size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
        size_t page_zero_bytes = PGSIZE - page_read_bytes;
        
        //s->pinned = true;
        /* Get a page of memory. */
        void* kpage = falloc (PAL_USER, s);
        if (kpage == NULL)
            return false;

        /* Load this page. */
        lock_acquire(&filesys_lock);
        if (file_read(file, kpage, page_read_bytes) != (int) page_read_bytes)
        {  
            lock_release(&filesys_lock);
            //printf("111111\n\n");
            frame_free (kpage);
            sys_exit(-1);
            return false; 
        }   
        lock_release(&filesys_lock);
        memset (kpage + page_read_bytes, 0, page_zero_bytes);
        /* Add the page to the process's address space. */
        if (!install_page_s (upage, kpage, writable)) 
        {   
            printf("22222222222222\n\n");
            frame_free (kpage);
            return false; 
        }   
    }
    //if(!intr_syscall)
     //   s->pinned = false;
    return true;
}
//replace load_segment
bool lazy_load_segment(struct file *file, off_t ofs_, uint8_t *upage,
        uint32_t read_bytes, uint32_t zero_bytes, bool writable)
{
    ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
    ASSERT (pg_ofs (upage) == 0);
    ASSERT (ofs_ % PGSIZE == 0);
    struct spte *s;
    bool success;
    off_t ofs = ofs_;
    file_seek (file, ofs);
    while (read_bytes > 0 || zero_bytes > 0)
    {
        /* Calculate how to fill this page.
         *          We will read PAGE_READ_BYTES bytes from FILE
         *                   and zero the final PAGE_ZERO_BYTES bytes. */
        size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
        size_t page_zero_bytes = PGSIZE - page_read_bytes;
        
        s = (struct spte*)malloc(sizeof(struct spte));
        s->type = file_t;
        s->vaddr = upage;
        s->t = thread_current();
        s->in_swap = false;
        s->pinned = false;
        s->file = file;
        s->offset = ofs;
        s->read_byte = page_read_bytes;
        s->zero_byte = page_zero_bytes;
        s->writable = writable;
        
        success = !hash_insert(&thread_current()->spt, &s->elem);
        if (!success)
        {
            PANIC("hash insert fail in lazy load segment\n");
        }
        //load_from_file(s);
        /* Advance. */
        read_bytes -= page_read_bytes;
        zero_bytes -= page_zero_bytes;
        ofs += page_read_bytes;
        //ofs += page_zero_bytes;
        upage += PGSIZE;

    }
    return true;
}
bool make_stack_page(void **esp)
{
    struct spte* s;
    bool success= false;
    uint8_t * kpage;
    *esp = PHYS_BASE;
    s = (struct spte*)malloc(sizeof(struct spte));
    s->type = stack_t;
    s->vaddr = *esp - PGSIZE;
    s->t = thread_current();
    s->pinned = false;
    s->in_swap = false;
    s->writable = true;

    kpage = falloc(PAL_USER | PAL_ZERO, s);
    if(kpage != NULL)
    {
        success = install_page_s(((uint8_t *) PHYS_BASE) - PGSIZE, kpage, true);
        if(!success)
        {
            frame_free(kpage);
            free(s);
        }
        else
        {
            thread_current()->esp = *esp;
            success = !hash_insert(&thread_current()->spt, &s->elem);
            s->pinned = false;
            if(!success)
                PANIC("fail to hash_insert -make_stack_page-\n");
        }
    }
    else
    {
        free(s);
    }
    return success;

}
bool grow_stack(void* fault_addr)
{
   if(fault_addr < STACK_END)
        PANIC("not stack area\n");
    struct spte* s;
    bool success;
    uint8_t * kpage;
    s = (struct spte*)malloc(sizeof(struct spte));
    s->type = stack_t;
    s->vaddr = pg_round_down(fault_addr);
    s->t = thread_current();
    s->pinned = false;
    s->in_swap =false;
    s->writable = true;
    kpage = falloc(PAL_USER | PAL_ZERO, s);
    if(kpage != NULL)
    {
        success = install_page_s( pg_round_down(fault_addr) , kpage, true);
        if(!success)
        {
            printf("gs install page fail\n");
            frame_free(kpage);
            free(s);
        }
        else
        {
            //thread_current()->esp = pg_round_up(fault_addr);
            success = !hash_insert(&thread_current()->spt, &s->elem);
            if(!success)
                PANIC("fail to hash_insert -make_stack_page-\n");
        }
    }
    else
    {
        printf("gs falloc fail\n");
        free(s);
    }
    if(!intr_syscall)
        s->pinned = false;
    return success;
    
}
int mmap_file(struct file* file, void* addr, int size)
{
    struct mmap_file * mf = (struct mmap_file*)malloc(sizeof(struct mmap_file));
    struct thread* t = thread_current();
    struct spte* s;
    bool success;
    off_t ofs;
    uint8_t* upage;
    uint32_t read_bytes;
    uint32_t zero_bytes;
    bool writable;
    //initalize
    mf->file = file;
    mf->map_id = t->mmap_num++;
    t->mmap_cnt++;
    mf->size = size;
    mf->addr = addr;
    mf->t = t;
    list_init(&mf->spte_list);
    t->mmap_list[mf->map_id] = mf;
    //set up
    ofs = 0;
    upage = addr;
    read_bytes = mf->size;
    zero_bytes = PGSIZE -(mf->size % PGSIZE);
    writable = true;

   while (read_bytes > 0 || zero_bytes > 0)
    {
        /* Calculate how to fill this page.
         *          We will read PAGE_READ_BYTES bytes from FILE
         *                   and zero the final PAGE_ZERO_BYTES bytes. */
        size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
        size_t page_zero_bytes = PGSIZE - page_read_bytes;
        
        s = (struct spte*)malloc(sizeof(struct spte));
        s->type = mmap_t;
        s->vaddr = upage;
        s->t = thread_current();
        s->in_swap = false;
        s->pinned = false;
        s->file = file;
        s->offset = ofs;
        s->read_byte = page_read_bytes;
        s->zero_byte = page_zero_bytes;
        s->writable = writable;
        
        success = !hash_insert(&thread_current()->spt, &s->elem);
        if (!success)
        {
            //over laping
            //we have to free mmaped things
            //delete overlaped entry
            free(s);
            munmap_file(mf->map_id);
            return -1;
           // PANIC("hash insert fail in lazy load segment\n");
        }
         list_push_back(&mf->spte_list, &s->l_elem);
        //load_from_file(s);
        /* Advance. */
        read_bytes -= page_read_bytes;
        zero_bytes -= page_zero_bytes;
        ofs += page_read_bytes;
        //ofs += page_zero_bytes;
        upage += PGSIZE;

    }
    return mf->map_id;
}
void munmap_file(int map_id)
{
    struct thread* t;
    struct mmap_file* mf;
    struct list_elem* e;
    struct spte* s;
    int i;
    uint32_t *pte;
    t = thread_current();
    mf = t->mmap_list[map_id];
    if(mf == NULL)
        PANIC("mf is NULL\n");
    pre_load(mf->addr,mf->size);
    for(e=list_begin(&mf->spte_list);e != list_end(&mf->spte_list);e= list_next(e))
    {
        s = list_entry(e, struct spte, l_elem);
        if(pagedir_is_dirty(t->pagedir, s->vaddr))
        {//write back to file
            //will need pre_load
            lock_acquire(&filesys_lock);
            file_write_at(s->file,s->vaddr,s->read_byte,s->offset);
            lock_release(&filesys_lock);
        }
        
        //have to delete spte and free frame?
       /* lock_acquire(&ft_lock);
        lock_acquire(&swap_lock);
        pte = lookup_page(s->t->pagedir,s->vaddr,false);
        if(pte != NULL && (*pte & 0x00000001))
        {//in frame
            frame_ffree(pagedir_get_page(s->t->pagedir,s->vaddr));
        }
        if(s->in_swap)
            clear_swap(s->swap_idx);
        lock_release(&swap_lock);
        
        hash_delete(&s->t->spt, &s->elem);
        free(s);
        lock_release(&ft_lock);
        */
    }
    lock_acquire(&filesys_lock);
    file_close(mf->file);
    lock_release(&filesys_lock);

    free_pin(mf->addr, mf->size);
    free(mf);
    t->mmap_list[map_id] = NULL;
    t->mmap_cnt--;
}
bool check_map(void* addr, int size)
{
    struct thread* t;
    struct mmap_file* mf;
    int i;
    t = thread_current();
    for(i=0;i<t->mmap_num;i++)
    {
        mf = t->mmap_list[i];
        if(!((unsigned)mf->addr >((unsigned)addr + (unsigned)size)||
                ((unsigned)mf->addr +(unsigned)mf->size)<(unsigned)addr))
            return true;
    }
    return false;
    // if overlap return true
}
void munmap_all()
{
    struct thread* t;
    struct mmap_file* mf;
    int i;
    t = thread_current();
    for(i=0;i<FD_MAX;i++)
    {
        if(t->mmap_list[i])
            munmap_file(i);
    }
}
