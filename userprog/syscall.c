#include <stdio.h>
#include <stdbool.h>
#include <syscall-nr.h>

#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/vaddr.h"

#include "userprog/syscall.h"
#include "userprog/process.h"

#include "lib/kernel/list.h"

#include "filesys/file.h"
#include "filesys/inode.h"
#include "filesys/filesys.h"
#include "filesys/directory.h"

#include "devices/input.h"
#include "devices/shutdown.h"

static void syscall_handler (struct intr_frame *);
static void copy_in (void *dst_, const void *usrc_, size_t size);
static bool get_user (uint8_t *dst, const uint8_t *usrc);
 
/* Inspired by this repo
   https://github.com/hfanc001/PintOS-Project-2/blob/master/pintos/src/userprog/syscall.c 
*/
static void
copy_in (void *dst_, const void *usrc_, size_t size)
{
  uint8_t *dst = dst_;
  const uint8_t *usrc = usrc_;

  for (; size > 0; size--, dst++, usrc++)
    if (usrc >= (uint8_t *) PHYS_BASE || !get_user (dst, usrc))
      thread_exit ();
}

static bool
get_user (uint8_t *dst, const uint8_t *usrc)
{
  int eax;
  asm ("movl $1f, %%eax; movb %2, %%al; movb %%al, %0; 1:"
       : "=m" (*dst), "=&a" (eax) : "m" (*usrc));
  return eax != 0;
}

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
  int argv[3];
  int arg_map[13] = { 0, 1, 1, 1, 2, 1, 1, 1, 3, 3, 2, 1, 1 };

  copy_in(argv, (uint32_t *) f->esp + 1, sizeof *argv * arg_map[system_call]);

  switch (system_call)
  {
  case SYS_HALT:
    halt ();
    break;
      
  case SYS_EXIT:
    exit (argv[0]);
    break;

  case SYS_EXEC:
    f->eax = exec ((const char*)argv[0]);
    break;

  case SYS_WAIT:
    f->eax = wait (argv[0]);
    break;

  case SYS_CREATE:
    f->eax = create ((const char *)argv[0], (unsigned)argv[1]);
    break;

  case SYS_REMOVE:
    f->eax = remove ((const char *)argv[0]);
    break;

  case SYS_OPEN:
    f->eax = open ((const char *)argv[0]);
    break;

  case SYS_FILESIZE:
    f->eax = filesize (argv[0]);
    break;

  case SYS_READ:
    f->eax = read (argv[0], (void *)argv[1], (unsigned)argv[2]);
    break;

  case SYS_WRITE:
    f->eax = write (argv[0], (void *)argv[1], (unsigned)argv[2]);
    break;

  case SYS_SEEK:
    seek (argv[0], (unsigned)argv[1]);
    break;

  case SYS_TELL:
    f->eax = tell (argv[0]);
    break;

  case SYS_CLOSE:
    close (argv[0]);
    break;

  default:
    thread_exit ();
    break;
  }
}

void halt()
{
  shutdown_power_off();
}

void exit(int status)
{
  thread_current()->parent->executed = true;
  thread_exit();
}

pid_t exec(const char *file)
{
  return -1;
}

int wait (pid_t pid) 
{
  return -1;
}

bool create (const char *file, unsigned initial_size) 
{
  return filesys_create(file, initial_size);
}

bool remove (const char *file) 
{
  return false;
}

int open (const char *file) 
{
  struct file_fd *fd_struct = malloc (4);
  fd_struct->f = filesys_open(file);
  
  if (fd_struct->f == NULL)
    return -1;
  
  struct thread* t = thread_current();

  list_push_back(&t->files,&fd_struct->elem);
  return list_size(&t->files);
}

int filesize (int fd) 
{
  return -1;
}

int read (int fd, void *buffer, unsigned size) 
{
  return -1;
}

int write (int fd, const void *buffer, unsigned size) 
{
  if (fd == 1) 
  {
    putbuf(buffer, size);
    return size;
  }

  struct file* f = file_open(fd);

  return file_write(f, buffer, size);
}

void seek (int fd, unsigned position) 
{

}

unsigned tell (int fd) 
{
  return 0;
}

void close (int fd) 
{

}
