#include "vm/swap.h"
#include <debug.h>
void swap_init()
{
    lock_init(&swap_lock);
   // printf("1212\n");
    swap_table.sd = block_get_role(BLOCK_SWAP);
   // printf("2323\n");
    if(swap_table.sd == NULL)
        return;

    // 1 page = 8 sector
    swap_table.bitmap = bitmap_create(block_size(swap_table.sd));
  //  printf("3434\n");
    if(swap_table.bitmap == NULL)
        PANIC("bitmap crate fail\n");
}
size_t swap_out(void* buf)
{
//write in swap disk
    size_t bit_idx;
    int i;
    lock_acquire(&swap_lock);
    // 1page = 8 sector
    if(swap_table.bitmap == NULL)
        PANIC("WHY??????!@!@!@\n");
    bit_idx = bitmap_scan_and_flip(swap_table.bitmap, 0, 8, false);
    lock_release(&swap_lock);

    if(bit_idx == BITMAP_ERROR)
        PANIC("swap disk is full");
    for(i = 0; i<8; i++)
        block_write(swap_table.sd, bit_idx + i, buf +(BLOCK_SECTOR_SIZE * i));


    return bit_idx;
//returns bitmap index
//bitmap index is managed by sector
}
void swap_in(void* buf, size_t bit_idx)
{
//read from swap
    int i;
    for(i = 0; i<8; i++)
        block_read(swap_table.sd, bit_idx + i, buf +(BLOCK_SECTOR_SIZE * i));
   
    lock_acquire(&swap_lock); 
    bitmap_set_multiple(swap_table.bitmap, bit_idx, 8, false);
    lock_release(&swap_lock);

}
