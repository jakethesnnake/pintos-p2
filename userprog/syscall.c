#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int * p = f->esp;
  int system_call = *p;
  printf("System Call: %d, %d\n", system_call, SYS_EXIT);

  switch (system_call)
  {
  case SYS_HALT:
    shutdown_power_off();
    break;
  
  case SYS_EXIT:
    thread_current()->parent->executed = true;
		thread_exit();
  
  default:
    break;
  }

  thread_exit ();
}
