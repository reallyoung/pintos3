#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include "threads/thread.h"
#include <stdint.h>
#include "threads/vaddr.h"

#define STACK_END (PHYS_BASE - (1 << 23))
// PHYS_BASE - 8MB
enum spte_t
{
    file_t = 0,
    mmap_t = 1,
    stack_t = 2
};

struct spte{
    enum spte_t type;
    bool pinned;
    void* vaddr;
    struct thread* t;
    bool in_swap;

    struct file* file;
    off_t offset;
    uint32_t read_byte;
    uint32_t zero_byte;
    bool writable;
    
    size_t swap_idx;

    struct hash_elem elem;
};
bool load_from_file(struct spte* s);
bool lazy_load_segment(struct file *file, off_t ofs, uint8_t *upage,
        uint32_t read_bytes, uint32_t zero_bytes, bool writable);
struct spte* get_spte(struct hash* spt, void* vaddr);
bool init_spt(struct hash* spt);
bool make_stack_page(void **esp);
void free_page_table(struct hash* spt);
bool grow_stack(void* fault_addr);
#endif
