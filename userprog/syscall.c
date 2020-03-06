#include <stdio.h>
#include <stdbool.h>
#include <syscall-nr.h>

#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"

#include "userprog/syscall.h"
#include "userprog/process.h"

#include "lib/kernel/list.h"

#include "filesys/file.h"
#include "filesys/inode.h"
#include "filesys/filesys.h"
#include "filesys/directory.h"
#include "filesys/off_t.h"

#include "devices/input.h"
#include "devices/shutdown.h"

static void syscall_handler (struct intr_frame *);
static void copy_in (void *dst_, const void *usrc_, size_t size);
static bool is_user (uint8_t *dst, const uint8_t *usrc);
 
/*
  Inspired by: https://github.com/ChristianJHughes/pintos-project2
  Ensures mutual exclusion in the file system
*/
struct lock file_lock;

/* Inspired by this repo
   https://github.com/hfanc001/PintOS-Project-2/blob/master/pintos/src/userprog/syscall.c 
*/
static void
copy_in (void *dst_, const void *usrc_, size_t size)
{
  uint8_t *dst = dst_;
  const uint8_t *usrc = usrc_;

  for (; size > 0; size--, dst++, usrc++)
    if (usrc >= (uint8_t *) PHYS_BASE || !is_user (dst, usrc))
      exit(-1);
}

static bool
is_user (uint8_t *dst, const uint8_t *usrc)
{
  int eax;
  asm ("movl $1f, %%eax; movb %2, %%al; movb %%al, %0; 1:"
       : "=m" (*dst), "=&a" (eax) : "m" (*usrc));
  return eax != 0;
}

struct file*
find_file(int fd, bool close)
{
  struct thread* t = thread_current();
  struct list_elem* e = list_head (&t->files);
  while ((e = list_next (e)) != list_end (&t->files)) 
  {
    struct file_fd *file_obj = list_entry (e, struct file_fd, elem);

    if (file_obj->fd == fd)
    {
      if (close)
        list_remove(e);
      return file_obj->f;
    }
  }

  return NULL;
}

void
syscall_init (void) 
{
  lock_init(&file_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
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
  thread_current()->exit_status = status;
  thread_exit();
}

pid_t exec(const char *cmd_line)
{
  if(cmd_line == NULL) return -1;

  lock_acquire(&file_lock);
  pid_t pid = process_execute(cmd_line);
  lock_release(&file_lock);
  return pid;
}

int wait (pid_t pid) 
{
  return process_wait(pid);
}

bool create (const char *file, unsigned initial_size) 
{
  if (file == NULL)
  {
    exit(-1);
    return false;
  }

  lock_acquire(&file_lock);
  bool was_created = filesys_create(file, initial_size);
  lock_release(&file_lock);
  return was_created;
}

bool remove (const char *file) 
{
  lock_acquire(&file_lock);
  bool was_removed = filesys_remove(file);
  lock_release(&file_lock);
  return was_removed;
}

int open (const char *file) 
{
  if (file == NULL) 
  {
    return -1;  
  }

  lock_acquire(&file_lock);
  struct file_fd *fd_struct = malloc (4); 
  fd_struct->f = filesys_open(file);

  if (fd_struct->f == NULL)
  {
    lock_release(&file_lock);
    return -1;
  }
  
  struct thread* t = thread_current();
  fd_struct->fd = t->file_count++;

  list_push_back(&t->files, &fd_struct->elem);
  lock_release(&file_lock);
  return fd_struct->fd;
}

int filesize (int fd) 
{
  lock_acquire(&file_lock);
  int len = (int) file_length(find_file(fd, false));
  lock_release(&file_lock);
  return len;
}

int read (int fd, void *buffer, unsigned size) 
{
  lock_acquire(&file_lock);
  if (!is_user_vaddr (buffer))
    exit (-1);

  if (fd == 0)
  {
    lock_release(&file_lock);
    return input_getc();
  }

  struct file* f = find_file(fd, false);

  if (f == NULL || buffer == NULL) {
    lock_release(&file_lock);
    return -1;
  }
    
  file_read(f, buffer, size);
  lock_release(&file_lock);
  
  return size;
}

int write (int fd, const void *buffer, unsigned size) 
{
  lock_acquire(&file_lock);
  if (fd == 1) 
  {
    putbuf(buffer, size);
    lock_release(&file_lock);
    return size;
  }

  struct file* f = find_file(fd, false);

  if (f == NULL) {
    lock_release(&file_lock);
    return -1;
  }

  lock_release(&file_lock);
  return file_write(f, buffer, size);
}

void seek (int fd, unsigned position) 
{
  lock_acquire(&file_lock);
  struct file *f = find_file(fd, false);
  f->pos = position;
  lock_release(&file_lock);
}

unsigned tell (int fd) 
{
  lock_acquire(&file_lock);
  off_t pos = find_file(fd, false)->pos;
  lock_release(&file_lock);
  return pos;
}

void close (int fd) 
{
  lock_acquire(&file_lock);
  struct file* f = find_file(fd, true);

  if (f != NULL)
    file_close(f);
  
  lock_release(&file_lock);
}
