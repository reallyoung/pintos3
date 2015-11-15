#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "threads/synch.h"
void syscall_init (void);
void sys_exit (int status);
struct lock filesys_lock;
bool intr_syscall;
#endif /* userprog/syscall.h */
