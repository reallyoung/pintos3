#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>

enum spte_t
{
    file_t = 0,
    swap_t = 1,
    mmap_t = 2
};

struct spte{
    enum spte_t type;
    bool pinned;
    void* vaddr;

    struct file* file;
    size_t offset;
    size_t read_byte;
    size_t zero_byte;

    struct hash_elem elem;
};
#endif
