#ifndef VM_SWAP_H
#define VM_SWAP_H
#include "devices/block.h"
#include <bitmap.h>
#include "threads/synch.h"

struct lock swap_lock;
struct lock disk_lock;
struct swap_table_{
    struct bitmap* bitmap;
    struct block* sd;

};
struct swap_table_ swap_table;
void swap_init();
size_t swap_out(void* buf);
void swap_in(void* buf, size_t bit_idx);
#endif
