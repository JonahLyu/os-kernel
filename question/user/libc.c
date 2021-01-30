/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */

#include "libc.h"

int  atoi( char* x        ) {
  char* p = x; bool s = false; int r = 0;

  if     ( *p == '-' ) {
    s =  true; p++;
  }
  else if( *p == '+' ) {
    s = false; p++;
  }

  for( int i = 0; *p != '\x00'; i++, p++ ) {
    r = s ? ( r * 10 ) - ( *p - '0' ) :
            ( r * 10 ) + ( *p - '0' ) ;
  }

  return r;
}

void itoa( char* r, int x ) {
  char* p = r; int t, n;

  if( x < 0 ) {
     p++; t = -x; n = t;
  }
  else {
          t = +x; n = t;
  }

  do {
     p++;                    n /= 10;
  } while( n );

    *p-- = '\x00';

  do {
    *p-- = '0' + ( t % 10 ); t /= 10;
  } while( t );

  if( x < 0 ) {
    *p-- = '-';
  }

  return;
}

void yield() {
  asm volatile( "svc %0     \n" // make system call SYS_YIELD
              :
              : "I" (SYS_YIELD)
              : );

  return;
}

int write( int fd, const void* x, size_t n ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 = fd
                "mov r1, %3 \n" // assign r1 =  x
                "mov r2, %4 \n" // assign r2 =  n
                "svc %1     \n" // make system call SYS_WRITE
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r) 
              : "I" (SYS_WRITE), "r" (fd), "r" (x), "r" (n)
              : "r0", "r1", "r2" );

  return r;
}

int  read( int fd,       void* x, size_t n ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 = fd
                "mov r1, %3 \n" // assign r1 =  x
                "mov r2, %4 \n" // assign r2 =  n
                "svc %1     \n" // make system call SYS_READ
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r) 
              : "I" (SYS_READ),  "r" (fd), "r" (x), "r" (n) 
              : "r0", "r1", "r2" );

  return r;
}

int  fork() {
  int r;

  asm volatile( "svc %1     \n" // make system call SYS_FORK
                "mov %0, r0 \n" // assign r  = r0 
              : "=r" (r) 
              : "I" (SYS_FORK)
              : "r0" );

  return r;
}

void exit( int x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 =  x
                "svc %0     \n" // make system call SYS_EXIT
              :
              : "I" (SYS_EXIT), "r" (x)
              : "r0" );

  return;
}

void exec( const void* x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 = x
                "svc %0     \n" // make system call SYS_EXEC
              :
              : "I" (SYS_EXEC), "r" (x)
              : "r0" );

  return;
}

int  kill( int pid, int x ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 =  pid
                "mov r1, %3 \n" // assign r1 =    x
                "svc %1     \n" // make system call SYS_KILL
                "mov %0, r0 \n" // assign r0 =    r
              : "=r" (r) 
              : "I" (SYS_KILL), "r" (pid), "r" (x)
              : "r0", "r1" );

  return r;
}

void nice( int pid, int x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 =  pid
                "mov r1, %2 \n" // assign r1 =    x
                "svc %0     \n" // make system call SYS_NICE
              : 
              : "I" (SYS_NICE), "r" (pid), "r" (x)
              : "r0", "r1" );

  return;
}

//IPC
int create_pipe( pid_t pid1, pid_t pid2){
  int r;
  
  asm volatile( "mov r0, %2 \n" // assign r0 =  pid1
                "mov r1, %3 \n" // assign r1 =  pid2
                "svc %1     \n" // make system call SYS_CREATE_PIPE
                "mov %0, r0 \n" // assign r0 =    r
              : "=r" (r) 
              : "I" (SYS_CREATE_PIPE), "r" (pid1), "r" (pid2)
              : "r0", "r1" );

  return r;
}


int open_pipe(pid_t pid1, pid_t pid2){
  int r;
  
  asm volatile( "mov r0, %2 \n" // assign r0 =  pid1
                "mov r1, %3 \n" // assign r1 =  pid2
                "svc %1     \n" // make system call SYS_OPEN_PIPE
                "mov %0, r0 \n" // assign r0 =    r
              : "=r" (r) 
              : "I" (SYS_OPEN_PIPE), "r" (pid1), "r" (pid2)
              : "r0", "r1" );

  return r;
}

void write_pipe(int i, int x){
  asm volatile( "mov r0, %1 \n" // assign r0 =  i
                "mov r1, %2 \n" // assign r1 =    x
                "svc %0     \n" // make system call SYS_OPEN_PIPE
              : 
              : "I" (SYS_WRITE_PIPE), "r" (i), "r" (x)
              : "r0", "r1" );

  return;
}

int read_pipe(int i){
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 =  i
                "svc %1     \n" // make system call SYS_READ_PIPE
                "mov %0, r0 \n" // assign r0 =    r
              : "=r" (r) 
              : "I" (SYS_READ_PIPE), "r" (i)
              : "r0", "r1" );

  return r;
}

void close_pipe(pid_t pid1, pid_t pid2){
  asm volatile( "mov r0, %1 \n" // assign r0 =  pid1
                "mov r1, %2 \n" // assign r1 =  pid2
                "svc %0     \n" // make system call SYS_CLOSE_PIPE
              : 
              : "I" (SYS_CLOSE_PIPE), "r" (pid1), "r" (pid2)
              : "r0", "r1" );

  return;
}

int get_pid(){
  int r;

  asm volatile( "svc %1     \n" // make system call SYS_FORK
                "mov %0, r0 \n" // assign r  = r0 
              : "=r" (r) 
              : "I" (SYS_GET_PID)
              : "r0" );

  return r;
}

void change_sche( int alg_index ) {
  asm volatile( "mov r0, %1 \n" // assign r0 =  x
                "svc %0     \n" // make system call SYS_CHANGE_SCHE
              :
              : "I" (SYS_CHANGE_SCHE), "r" (alg_index)
              : "r0" );

  return;
}

void state() {
  asm volatile( "svc %0     \n" // make system call SYS_STATE
              :
              : "I" (SYS_STATE)
              : );

  return;
}

char* list_file(){
  char* r;

  asm volatile( "svc %1     \n" // make system call SYS_LIST_FILE
                "mov %0, r0 \n" // assign r  = r0 
              : "=r" (r) 
              : "I" (SYS_LIST_FILE)
              : "r0" );
  return r;
}

int disk_block_num(){
  int r;

  asm volatile( "svc %1     \n" // make system call SYS_FORK
                "mov %0, r0 \n" // assign r  = r0 
              : "=r" (r) 
              : "I" (SYS_DISK_BLOCK_NUM)
              : "r0" );

  return r;
}

int disk_block_len(){
  int r;

  asm volatile( "svc %1     \n" // make system call SYS_FORK
                "mov %0, r0 \n" // assign r  = r0 
              : "=r" (r) 
              : "I" (SYS_DISK_BLOCK_LEN)
              : "r0" );

  return r;
}

uint32_t open(char* filename, int flag){
  uint32_t r;
  
  asm volatile( "mov r0, %2 \n" // assign r0 =  filename
                "mov r1, %3 \n" // assign r1 =  flag
                "svc %1     \n" // make system call SYS_OPEN
                "mov %0, r0 \n" // assign r0 =    r
              : "=r" (r) 
              : "I" (SYS_OPEN), "r" (filename), "r" (flag)
              : "r0", "r1" );

  return r;
}

void close(uint32_t fd){
  asm volatile( "mov r0, %1 \n" // assign r0 =  fd
                "svc %0     \n" // make system call SYS_CLOSE
              :
              : "I" (SYS_CLOSE), "r" (fd)
              : "r0" );

  return;  
}

int remove_file(char* filename){
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 =  filename
                "svc %1     \n" // make system call SYS_REMOVE_FILE
                "mov %0, r0 \n" // assign r0 =    r
              : "=r" (r) 
              : "I" (SYS_REMOVE_FILE), "r" (filename)
              : "r0", "r1" );

  return r;
}

//get the byte number (size) of a file
int word_count(int fd){
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 =  fd
                "svc %1     \n" // make system call SYS_WORD_COUNT
                "mov %0, r0 \n" // assign r0 =    r
              : "=r" (r) 
              : "I" (SYS_WORD_COUNT), "r" (fd)
              : "r0", "r1" );

  return r;
}
