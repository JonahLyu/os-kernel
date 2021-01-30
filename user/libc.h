/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */

#ifndef __LIBC_H
#define __LIBC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Define a type that that captures a Process IDentifier (PID).

typedef int pid_t;

/* The definitions below capture symbolic constants within these classes:
 *
 * 1. system call identifiers (i.e., the constant used by a system call
 *    to specify which action the kernel should take),
 * 2. signal identifiers (as used by the kill system call), 
 * 3. status codes for exit,
 * 4. standard file descriptors (e.g., for read and write system calls),
 * 5. platform-specific constants, which may need calibration (wrt. the
 *    underlying hardware QEMU is executed on).
 *
 * They don't *precisely* match the standard C library, but are intended
 * to act as a limited model of similar concepts.
 */

#define SYS_YIELD     ( 0x00 )
#define SYS_WRITE     ( 0x01 )
#define SYS_READ      ( 0x02 )
#define SYS_FORK      ( 0x03 )
#define SYS_EXIT      ( 0x04 )
#define SYS_EXEC      ( 0x05 )
#define SYS_KILL      ( 0x06 )
#define SYS_NICE      ( 0x07 )

#define SYS_CREATE_PIPE    ( 0x08 )
#define SYS_OPEN_PIPE      ( 0x09 )
#define SYS_WRITE_PIPE     ( 0x0A )
#define SYS_READ_PIPE      ( 0x0B )
#define SYS_CLOSE_PIPE     ( 0x0C )

#define SYS_GET_PID      ( 0x0D )
#define SYS_CHANGE_SCHE  ( 0x0E )
#define SYS_STATE        ( 0x0F )


#define SYS_DISK_BLOCK_NUM   ( 0x10 )
#define SYS_DISK_BLOCK_LEN   ( 0x11 )
#define SYS_OPEN             ( 0x12 )
#define SYS_LIST_FILE        ( 0x13 )
#define SYS_CLOSE            ( 0x14 )
#define SYS_REMOVE_FILE      ( 0x15 )
#define SYS_WORD_COUNT       ( 0x16 )

#define SIG_TERM      ( 0x00 )
#define SIG_QUIT      ( 0x01 )

#define EXIT_SUCCESS  ( 0 )
#define EXIT_FAILURE  ( 1 )

#define  STDIN_FILENO ( 0 )
#define STDOUT_FILENO ( 1 )
#define STDERR_FILENO ( 2 )

#define READ       (0x0)
#define WRITE      (0x1)
#define READ_WRITE (0x2)

// convert ASCII string x into integer r
extern int  atoi( char* x        );
// convert integer x into ASCII string r
extern void itoa( char* r, int x );

// cooperatively yield control of processor, i.e., invoke the scheduler
extern void yield();

// write n bytes from x to   the file descriptor fd; return bytes written
extern int write( int fd, const void* x, size_t n );
// read  n bytes into x from the file descriptor fd; return bytes read
extern int  read( int fd,       void* x, size_t n );

// perform fork, returning 0 iff. child or > 0 iff. parent process
extern int  fork();
// perform exit, i.e., terminate process with status x
extern void exit(       int   x );
// perform exec, i.e., start executing program at address x
extern void exec( const void* x );

// for process identified by pid, send signal of x
extern int  kill( pid_t pid, int x );
// for process identified by pid, set  priority to x
extern void nice( pid_t pid, int x );

//IPC
//create a pipe to connect processes identified by pid1 and pid2, return the pipe index
extern int create_pipe( pid_t pid1, pid_t pid2);
//open the pipe between the processes identified by pid1 and pid2, return the pipe index
extern int open_pipe(pid_t pid1, pid_t pid2);
//write integer x into pipe with index i
extern void write_pipe(int i, int x);
//read from pipe with index i
extern int read_pipe(int i);
//close pipe between the processes identified by pid1 and pid2
extern void close_pipe(pid_t pid1, pid_t pid2);

//Useful system calls
//get pid of current process;
extern int get_pid();
//change the scheduler algorithm with the corresponding index
extern void change_sche(int alg_index);
//print the current system state in shell
extern void state();
//list all current file
extern char* list_file();
//remove a file from the current directory
extern int remove_file(char* filename);
//find the number of byte of a file
extern int word_count(int fd);

//file system calls
//open a file, with flag(READ,WRITE,READ_WRITE),return file descriptor
extern uint32_t open(char* filename, int flag); 
//close a file with the file descriptor
extern void close(uint32_t fd); 


//disk system calls
//return the disk block num
extern int disk_block_num();
//return the disk block length
extern int disk_block_len();

#endif
