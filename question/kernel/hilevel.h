/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */

#ifndef __HILEVEL_H
#define __HILEVEL_H

// Include functionality relating to newlib (the standard C library).

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <string.h>

// Include functionality relating to the kernel.

#include "lolevel.h"
#include     "int.h"

// Include functionality relating to the platform.

#include   "GIC.h"
#include "PL011.h"
#include "SP804.h"
#include "disk.h"

#define READ       (0x0)
#define WRITE      (0x1)
#define READ_WRITE (0x2)

#define  STDIN_FILENO ( 0 )
#define STDOUT_FILENO ( 1 )
#define STDERR_FILENO ( 2 )

typedef int pid_t;

typedef int priority_t;

typedef char* directory_t;

typedef enum { 
  STATUS_CREATED,
  STATUS_READY,
  STATUS_EXECUTING,
  STATUS_WAITING,
  STATUS_TERMINATED
} status_t;


typedef struct {
  uint32_t cpsr, pc, gpr[ 13 ], sp, lr;
} ctx_t;

//struct of file descripter table
typedef struct{
  uint32_t fcb;
  int flag;
  uint32_t rw_pointer;
  uint32_t inode_id;
} fd_t;

typedef struct {
     pid_t       pid;
  status_t    status;
     ctx_t       ctx;
 priority_t  priority;
 priority_t  current_priority;
 directory_t  working_directory_name;
  uint32_t   working_directory_address;
       fd_t  fdt[10];
} pcb_t;

//scheduler algorithm
extern void sche_round_robin( ctx_t* ctx );
extern void sche_priority_based( ctx_t* ctx );

#endif
