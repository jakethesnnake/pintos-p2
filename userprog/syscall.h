#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>
#include <debug.h>

/* fd file stucture */
struct file_fd 
{
   int fd;
   struct file *f;
   struct list_elem elem;
};

typedef int pid_t;

void syscall_init (void);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
void halt();
void exit(int status);
pid_t exec(const char *file);
int wait (pid_t pid);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);

#endif /* userprog/syscall.h */
