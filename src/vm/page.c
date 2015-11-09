#include "vm/page.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/malloc.h"
#include "vm/frame.h"
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
    free(s);
}
bool init_spt(struct hash* spt)
{
   return hash_init(spt,spt_hash_func, spt_less_func, NULL);
}
static bool
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
    //unsigned mask = 0xfffff000;
    //ispte.vaddr = (void *)((unsigned)vaddr & mask);
    ispte.vaddr = pg_round_down(vaddr);

    e = hash_find(spt,&ispte.elem);
    
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
    ASSERT (ofs % PGSIZE == 0); 

    //file_seek (file, ofs);
    if (read_bytes > 0 || zero_bytes > 0)  
    {   
        /* Calculate how to fill this page.
         *          We will read PAGE_READ_BYTES bytes from FILE
         *                   and zero the final PAGE_ZERO_BYTES bytes. */
        size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
        size_t page_zero_bytes = PGSIZE - page_read_bytes;

        /* Get a page of memory. */
        void* kpage = falloc (PAL_USER);
        if (kpage == NULL)
            return false;

        /* Load this page. */
        if (file_read_at (file, kpage, page_read_bytes, ofs) != (int) page_read_bytes)
        {  
            printf("111111\n\n");
            frame_free (kpage);
            return false; 
        }   
        memset (kpage + page_read_bytes, 0, page_zero_bytes);

        /* Add the page to the process's address space. */
        if (!install_page_s (upage, kpage, writable)) 
        {   
            printf("22222222222222\n\n");
            frame_free (kpage);
            return false; 
        }   
    }
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
        s->pinned = false;
        s->file = file;
        s->offset = ofs;
        s->read_byte = read_bytes;
        s->zero_byte = zero_bytes;
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
