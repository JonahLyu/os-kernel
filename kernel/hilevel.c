/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */

#include "hilevel.h"

#define PCB_MAX_NUM 30
#define PROCESS_NUM 1
#define FORK_PRIORITY 1
#define CONSOLE_PRIORITY 1
#define PIPE_MAX_SIZE 2
#define PIPE_MAX_NUM 50
#define MAX_FILE_NUM 15

bool isprint = true;

pcb_t pcb[ PCB_MAX_NUM ]; 
pcb_t* current = NULL;
int pcb_count;
int new_pid = 0;
void (*schedule_alg)(ctx_t*) = &sche_priority_based;  //the initail scheduler algorithm pointer


typedef struct Pipe{
  pid_t source_pid;
  pid_t dest_pid;
  int pipe[PIPE_MAX_SIZE];
  int head;
  int tail;
  int length;
}pipe_t;

pipe_t all_pipes[PIPE_MAX_NUM];

int pipe_count = 0;   //number of pipe being used

typedef struct File{
  char* filename;
  uint32_t inode_id;
}file_t;
file_t filename_table[MAX_FILE_NUM];

void enqueue(int *q,int *tail, int element, int *length){
  if(*length != PIPE_MAX_SIZE) {
    if(*tail == PIPE_MAX_SIZE-1) {
       *tail = -1;            
    }       
    q[++(*tail)] = element;
    (*length)++;
  }
}

int dequeue(int *q, int *head, int *length){
 int data = q[(*head)++];

 if(*head == PIPE_MAX_SIZE) {
    *head = 0;
 }
 (*length)--;
 return data; 
}

//find an empty space in pcb list
int findEmpty(){
  for (int i = 0; i < PCB_MAX_NUM; i++){
    if (pcb[i].status == STATUS_TERMINATED) return i;  
  }
  return -1;
}

//initialise file descriptor table of a process
void initialise_fdt(int index){
strcpy(pcb[ index ].working_directory_name, "/\0");
  pcb[ index ].working_directory_address = 0x00001000;
  pcb[ index ].fdt[0].fcb = STDIN_FILENO;
  pcb[ index ].fdt[0].flag = READ;
  pcb[ index ].fdt[0].rw_pointer = 0;
  pcb[ index ].fdt[0].inode_id = 0;
  pcb[ index ].fdt[1].flag = WRITE;
  pcb[ index ].fdt[1].fcb = STDOUT_FILENO;
  pcb[ index ].fdt[1].rw_pointer = 0;
  pcb[ index ].fdt[1].inode_id = 0;
  pcb[ index ].fdt[2].fcb = STDERR_FILENO;   
  pcb[ index ].fdt[2].flag = WRITE;
  pcb[ index ].fdt[2].rw_pointer = 0;
  pcb[ index ].fdt[2].inode_id = 0;
  //set all empty fcb pointer to -1
  for (int i = 3; i < 10;i++) pcb[ index ].fdt[i].rw_pointer = -1;
}

//find an empty inode, return inode the number
uint32_t findEmptyInode(){
  uint8_t* data = (uint8_t*) malloc(16);
  for (int i = 0; i < MAX_FILE_NUM; i++){   //MAX_FILE_NUM files
    disk_rd(0x00002000 + i, data, 16);
    if (*data == 0) return 0x00002000 + i;//find an empty inode   
  }
}

//find a file, return inode id (the address of inode)
uint32_t findFile(char* filename){
  //check file name table first
  for (int i = 0; i < MAX_FILE_NUM; i++){
    if (0 == strcmp(filename_table[i].filename, filename)) return filename_table[i].inode_id;   
  }
  
  uint8_t* data = (uint8_t*) malloc(17); //file name is stored in one block
//   for (int i = 0; i < MAX_FILE_NUM; i++){   //MAX_FILE_NUM files
//     disk_rd(0x00001000 + i * 2, data, 16);
//     if (0 == strcmp(data, filename)) {
//       disk_rd(0x00001000 + i * 2 + 1, data, 16);  //the inode id is stored in next block
//       uint32_t inode_id = 0;
//       data++; //the first byte is flag
//       inode_id = ((inode_id + *data) << 8) & 0xFF00;
//       data++;
//       inode_id += *data;   
//       return inode_id;
//     }
//   }
  //create an empty file
  for (int i = 0; i < MAX_FILE_NUM; i++){   //MAX_FILE_NUM files
    disk_rd(0x00001000 + i * 2 + 1, data, 16);
    if (*data == 0x0) {//this is an empty file space
      disk_wr(0x00001000 + i * 2, filename, 16);
      uint32_t inode_id = findEmptyInode();
      //insert new file entry into file name table
      for (int j = 0; j < MAX_FILE_NUM; j++){
        if (filename_table[j].inode_id == 0){
          strcpy(filename_table[j].filename, filename);
          filename_table[j].inode_id = inode_id;
          break;
        }
      }
      //write file name into disk
      memset(data, 0 , 16);
      data[0] = 1;
      data[1] = (uint8_t) (inode_id >> 8) & 0x000000FF;  //write inode id to file block
      data[2] = (uint8_t) inode_id & 0x000000FF;
      disk_wr(0x00001000 + i * 2 + 1, data, 16);
      return inode_id;
    }
  }
  return 0;
}

//find a new Data block Address
uint32_t newDataAddress(uint32_t inode_id){
  uint8_t * data =  (uint8_t*) malloc(16);
  uint32_t max_address = 0x00004000;
  uint32_t address;

  address = max_address + (inode_id - 0x2000) * 0x00000100; //assume each file takes 256 blocks
  data[0] = 1; //flag
  data[1] = 0; //size
  data[2] = 0;
  data[3] = (uint8_t) (address >> 8) & 0x000000FF;  //write inode id to file block
  data[4] = (uint8_t) address & 0x000000FF;
  disk_wr(inode_id, data, 16);
  return address;
}

//find inode and return the address to the data block
uint32_t findData(uint32_t inode_id){
  uint8_t* data = (uint8_t*) malloc(16 + 1); 
  disk_rd(inode_id, data, 16);
  
  data += 3;  //point to the block pointer
  uint32_t address = (0 + *data) << 8;
  data++;
  address += *data;
  //inode point to empty file
  if (address < 0x4000) {
    address = newDataAddress(inode_id);
  }
  return address;
}

//get the file size from inode meta-data
int getFileSize(int fd){
  uint8_t* inode = (uint8_t *) malloc(16 + 1);
  disk_rd(current->fdt[fd].inode_id, inode, 16);
  inode++;
  int size = (0 + *inode) << 8;
  inode++;
  size += *inode;
  return size;
}

//update file size to n
void updateFileSize(int fd, int n){
  uint8_t* inode = (uint8_t *) malloc(16 + 1);
  disk_rd(current->fdt[fd].inode_id, inode, 16);
  inode[1] = (n >> 8) & 0x000000FF;
  inode[2] = n & 0x000000FF;
  disk_wr(current->fdt[fd].inode_id, inode, 16);
}

//unused
uint32_t hashaddress(char* filename){
  uint32_t i = 0;
  while(*filename != '\0'){
    i += *filename;
    filename++;
  }
  return (uint32_t)0x1000 + i % 0x1000;
}


void dispatch( ctx_t* ctx, pcb_t* prev, pcb_t* next ) {
  char prev_pid = '?', next_pid = '?';

  if( NULL != prev ) {
    memcpy( &prev->ctx, ctx, sizeof( ctx_t ) ); // preserve execution context of P_{prev}
    prev_pid = '0' + prev->pid;
  }
  if( NULL != next ) {
    memcpy( ctx, &next->ctx, sizeof( ctx_t ) ); // restore  execution context of P_{next}
    next_pid = '0' + next->pid;
  }
//   if (isprint){
//     PL011_putc( UART0, '[',      true );
//     PL011_putc( UART0, prev_pid, true );
//     PL011_putc( UART0, '-',      true );
//     PL011_putc( UART0, '>',      true );
//     PL011_putc( UART0, next_pid, true );
//     PL011_putc( UART0, ']',      true );
//     char a = '0' + pcb[0].status;
//     char b = '0' + pcb[1].status;
//     char c = '0' + pcb[2].status;
//     char d = '0' + pcb[3].status;
//     char e = '0' + pcb[4].status;
//     char f = '0' + pcb[5].status;
//     PL011_putc( UART0, '<',      true );
//     PL011_putc( UART0, a,      true );
//     PL011_putc( UART0, b,      true );
//     PL011_putc( UART0, c,      true );
//     PL011_putc( UART0, d,      true );
//     PL011_putc( UART0, e,      true );
//     PL011_putc( UART0, f,      true );
//     PL011_putc( UART0, '>',      true );
//     PL011_putc( UART0, '{',      true );
//     PL011_putc( UART0, pcb_count + '0',      true );
//     PL011_putc( UART0, '}',      true );
//   } 
    current = next;                             // update   executing index   to P_{next}

  return;
}

void sche_round_robin( ctx_t* ctx ) {
  int index;
  for (int i = 0; i < PCB_MAX_NUM; i++){
    if (current->pid == pcb[ i ].pid) {
      index = i;
      break;
    }
  }
  int i = index + 1;
  while(i != index){
    if (pcb[ i ].status != STATUS_READY && pcb[ i ].status != STATUS_CREATED) {
      i = (i + 1) % PCB_MAX_NUM;
      continue;
    }
    if (current->status == STATUS_TERMINATED) current->status = STATUS_TERMINATED; //exit successfully
    else current->status = STATUS_READY;        // update   execution status  of P_n
    pcb[ i ].status = STATUS_EXECUTING;             // update   execution status  of P_n+1
    dispatch( ctx, current, &pcb[ i ] );      // context switch P_n -> P_n+1
    return;
  }
  
  current->status = STATUS_EXECUTING; 
  dispatch( ctx, current, current );   //only one process
  return;
}

void sche_priority_based( ctx_t* ctx ) {
  
  //update all priorities
  for (int i = 0; i < PCB_MAX_NUM; i++){
    if (pcb[ i ].status == STATUS_TERMINATED) continue;
    if (current->pid == pcb[ i ].pid) pcb[ i ].current_priority = pcb[ i ].priority;
    else pcb[ i ].current_priority ++;
  }  
  
  //if console does not run in 3 rounds, dispatch to console
  if ((pcb[ 0 ].current_priority - pcb[ 0 ].priority) > 3) {  
    if (current->status != STATUS_TERMINATED) current->status = STATUS_READY;
    dispatch( ctx, current, &pcb[ 0 ] );
    return;
  }
  
  int new_index;
  //find the max priority
  int max_p = 0;
  for (int i = 0; i < PCB_MAX_NUM; i++){
    if (pcb[ i ].status == STATUS_TERMINATED) continue;
    if (pcb[ i ].current_priority > max_p){ 
      max_p = pcb[ i ].current_priority;
      new_index = i;
    }
  } 
  if (current->status != STATUS_TERMINATED) current->status = STATUS_READY; 
  pcb[new_index].status = STATUS_EXECUTING; 
  dispatch( ctx, current, &pcb[ new_index ] ); 
}

extern void     main_console();
extern void     main_P3(); 
extern void     main_P4(); 
extern void     main_P5(); 
extern void     main_P6(); 
extern void     main_P7(); 
extern void     main_Dining(); 
extern void     test_write_file(); 
extern void     test_read_file(); 
extern void     test_wc(); 

extern uint32_t tos_console;
extern uint32_t bos_UP;

void hilevel_handler_rst(ctx_t* ctx) {
  //initialise fdt of console
  initialise_fdt(0);
  
  //initialise file name record table
  for (int i = 0; i < MAX_FILE_NUM; i++) {
    filename_table[i].filename = (char*) malloc(16);
    memset(filename_table[i].filename, 0, 16);
    filename_table[i].inode_id = 0;
  }
  uint8_t* filename = (uint8_t*) malloc(16);
  uint32_t inode_id;
  for (int i = 0; i < MAX_FILE_NUM; i++){   //MAX_FILE_NUM files
    disk_rd(0x00001000 + i * 2 + 1, filename, 16);
    if (*filename == 0x1) {
      inode_id = 0;
      inode_id = (0 + filename[1]) << 8;
      inode_id += filename[2];
      filename_table[i].inode_id = inode_id;
      memset(filename, 0, 16);
      disk_rd(0x00001000 + i * 2, filename, 16);
      strcpy(filename_table[i].filename, filename);       
    }
  }
    
  TIMER0->Timer1Load  = 0x00100000; // select period = 2^20 ticks ~= 1 sec
  TIMER0->Timer1Ctrl  = 0x00000002; // select 32-bit   timer
  TIMER0->Timer1Ctrl |= 0x00000040; // select periodic timer
  TIMER0->Timer1Ctrl |= 0x00000020; // enable          timer interrupt
  TIMER0->Timer1Ctrl |= 0x00000080; // enable          timer

  GICC0->PMR          = 0x000000F0; // unmask all            interrupts
  GICD0->ISENABLER1  |= 0x00000010; // enable timer          interrupt
  GICC0->CTLR         = 0x00000001; // enable GIC interface
  GICD0->CTLR         = 0x00000001; // enable GIC distributor
    
  int_enable_irq();

  //run console.c
  memset( &pcb[ 0 ], 0, sizeof( pcb_t ) );     // initialise 0-th PCB = console
  pcb[ 0 ].pid      = new_pid;
  pcb[ 0 ].status   = STATUS_CREATED;
  pcb[ 0 ].ctx.cpsr = 0x50;
  pcb[ 0 ].ctx.pc   = ( uint32_t )( &main_console);
  pcb[ 0 ].ctx.sp   = ( uint32_t )( &tos_console  );
  pcb[ 0 ].priority = CONSOLE_PRIORITY;
  pcb[ 0 ].current_priority = pcb[ 0 ].priority;
  pcb_count = 1;  //pcb number sets to one
  
     
  //set all other empty pcb to terminated with pid -1
  for (int i = PROCESS_NUM; i < PCB_MAX_NUM; i++) {
      pcb[ i ].pid = -1;
      pcb[ i ].status = STATUS_TERMINATED;    
  }
  
  //initialise pipes
  for (int i = 0; i < PIPE_MAX_NUM; i++){
    memset( &all_pipes[ i ], 0, sizeof( pipe_t ) );
    all_pipes[i].source_pid = -1;
    all_pipes[i].dest_pid = -1;
  } 
    
  dispatch( ctx, NULL, &pcb[ 0 ] );
    
  return;
}

void hilevel_handler_irq(ctx_t* ctx) {
  //handle the timer interrupt
  uint32_t id = GICC0->IAR;
  
  if( id == GIC_SOURCE_TIMER0 ) {
//     if (isprint){
//       PL011_putc( UART0, 'T', true ); 
//     }
    TIMER0->Timer1IntClr = 0x01;
  }
  (*schedule_alg)(ctx);  //invoke the scheduler

  GICC0->EOIR = id;
    
  return;
}

void hilevel_handler_svc(ctx_t* ctx, uint32_t id) {
  switch( id ) {
    case 0x00 :{ // 0x00 => yield()
      (*schedule_alg)(ctx);
      break;
    }
    case 0x01 :{ // 0x01 => write( fd, x, n )
      int   fd = ( int   )( ctx->gpr[ 0 ] );  
      char*  x = ( char* )( ctx->gpr[ 1 ] );  
      int    n = ( int   )( ctx->gpr[ 2 ] ); 
      int num = 0;
      bool success = true;
      if (fd == 1){  //fd = STDOUT
        for( int i = 0; i < n; i++ ) {
          PL011_putc( UART0, *x++, true );
        }
        ctx->gpr[ 0 ] = n;
      } 
      else{   
        
        int p = current->fdt[fd].rw_pointer;
        uint32_t address = current->fdt[fd].fcb + ( p / 16);           
        if (p % 16 + n < 16){    //write data within one block  
          uint8_t* data = (uint8_t *) malloc(16 + 1);
          disk_rd(address, data, 16);
          for (int i=0; i < n; i++) data[p % 16 + i] = x[i];
          int r = disk_wr(address, data, 16);
          if (r == -1) ctx->gpr[ 0 ] = 0;
          else {
            current->fdt[fd].rw_pointer += n;
            ctx->gpr[ 0 ] = n;
          }
          break;
        }
        //write data among multiple blocks
        uint8_t* data = (uint8_t *) malloc(n + p % 16 + 1);
        disk_rd(address, data, 16);
        data[p % 16] = '\0';
        strcat(data, x);
        int length = n + p % 16;  
        for (int i = 0; i< length /16; i++){ 
          int r = disk_wr(address + i, data, 16);
          if (r == -1) success = false;
          data += 16;
        }
        uint8_t* rem_data = (uint8_t *) malloc(16 + 1);
        disk_rd(address + length /16, rem_data, 16);
        for (int i = 0; i < length % 16; i++) rem_data[i] = data[i];
        int r = disk_wr(address + length /16, rem_data, 16);
        if (r == -1) success = false;

        if (success) {
          ctx->gpr[ 0 ] = n;
          //update the file size
          int size = getFileSize(fd);
          if (current->fdt[fd].rw_pointer + n > size){
            updateFileSize(fd, size + n);
          }
          //update pointer
          current->fdt[fd].rw_pointer += n;
        }
        else ctx->gpr[ 0 ] = 0;
        break;                    
      }
      break;
    }
    case 0x02 :{ // 0x02 => read( fd, x, n )
      int   fd = ( int   )( ctx->gpr[ 0 ] );  
      char*  x = ( char* )( ctx->gpr[ 1 ] );  
      int    n = ( int   )( ctx->gpr[ 2 ] ); 
      bool success = true;
      if (current->fdt[fd].flag == WRITE) {  //can not read in write mode
        ctx->gpr[0] = 0;
        return;
      }
      
      if (fd == 0){  //fd = STDIN
        for( int i = 0; i < n; i++ ) {
          x[ i ] = PL011_getc( UART0, true );
        }        
        ctx->gpr[ 0 ] = n;
      }
      else{
         int p = current->fdt[fd].rw_pointer;
         int num = 0;   //record how many address read
         uint32_t address = current->fdt[fd].fcb + ( p / 16);  //set the inital read address
         uint8_t * data = (uint8_t*) malloc(p % 16 + n + 16); 
         for (int i = 0; i < (p % 16 + n) /16 + 1; i++){  //read all data
           int r = disk_rd(address + i, data, 16);
           if (r == -1) {
             success = false;  //if read fail break
             break;
           }
           data += 16;
           num++;
         }            
         if (success) {
           current->fdt[fd].rw_pointer += n; 
           data = data - 16 * num;   //shift pointer to the beginning
           data += p % 16;
           strcpy(x, data);
           ctx->gpr[0] = n;
         }
         else ctx->gpr[0] = 0;
         break;
      }             
      break;
    }
    case 0x03 :{ // 0x03 => fork()
      if (pcb_count == PCB_MAX_NUM) break;      //system has reached maximum pcb number
      int index = findEmpty();
      memset( &pcb[ index ], 0, sizeof( pcb_t ) );    
      pcb[ index ].pid      = index;
      pcb[ index ].status   = STATUS_READY;
      memcpy( &pcb[ index ].ctx, ctx, sizeof( ctx_t ) ); //copy ctx of parent to child

      pcb[ index ].ctx.gpr[0] = 0;                      //set child return value
      pcb[ index ].ctx.sp   = ( uint32_t )( &bos_UP + 0x00001000 * pcb[ index ].pid); //assign the child a stack
      pcb[ index ].priority = FORK_PRIORITY;
      pcb[ index ].current_priority = pcb[ index ].priority;

      initialise_fdt(index);

      pcb_count++; //process number plus one

      ctx->gpr[ 0 ] = pcb[ index ].pid;   //set parent return value
      break;
    } 
    case 0x04 :{ // 0x04 => exit(x)
      int exit_status = ctx->gpr[0];
      if (exit_status == 0){ //exit successfully
        current->status = STATUS_TERMINATED; //set current process status to terminated 
        current->pid = -1; //set current process pid to -1     
        pcb_count = 0;
        for (int i=0; i < PCB_MAX_NUM; i++){
          if (pcb[i].status != STATUS_TERMINATED) pcb_count++;
        }
      }
      (*schedule_alg)(ctx);
      break;      
    }         
    case 0x05 :{ // 0x05 => exec(x)
      void* p = (void*) ctx->gpr[0]; 
      if (p == NULL) {
        current->pid = -1;   //set current process pid to -1
        current->status = STATUS_TERMINATED;  //set current process status to terminated 
        pcb_count--;   //process number minus one
        return; //there is no program to be executed
      }
      //initialise priority of the process
      current->priority = 1;       

      current->current_priority = current->priority;  //initialise current priority  

      ctx->sp = ( uint32_t )( &bos_UP + 0x00001000 * current->pid );   //set ctx sp to the top of stack
      ctx->pc = ( uint32_t ) p;  //set pc to new program
      break;
    }
    case 0x06 :{ // 0x06 => kill( pid, x )
      int pid = ctx->gpr[0];
      int x = ctx->gpr[1];
      if ( pid > 0 && pid < PCB_MAX_NUM) {
        pcb[pid].pid = -1;   //set current process pid to -1
        if(pcb[pid].status != STATUS_TERMINATED) {  //check if this pid has already been killed
          pcb[pid].status = STATUS_TERMINATED;
          pcb_count--;   //process number minus one
        }     
      }       
      break;       
    } 
    case 0x07 :{ // 0x07 => nice( pid, x )
      int pid = ctx->gpr[0];
      int x = ctx->gpr[1];
      if (pid >=0 && pid < PCB_MAX_NUM) pcb[pid].priority = x;
      break; 
    }
    case 0x08 :{ // 0x08 => create_pipe(source_pid, dest_pid)
      int source_pid = ctx->gpr[0];
      int dest_pid = ctx->gpr[1];
      if (pipe_count == PIPE_MAX_NUM) { //no empty space for new pipe, return -1
        ctx->gpr[0] = -1;
        return;
      }
      for (int i = 0; i < PIPE_MAX_NUM; i++){  //check if a pipe exists
        if (all_pipes[i].source_pid == source_pid && all_pipes[i].dest_pid == dest_pid){
          ctx->gpr[0] = i; 
          return;
        }
      }

      for (int i = 0; i < PIPE_MAX_NUM; i++){ //find an empty pipe
        if (all_pipes[i].source_pid == -1){
          all_pipes[i].source_pid = source_pid;
          all_pipes[i].dest_pid = dest_pid;
          all_pipes[i].head = 0;
          all_pipes[i].tail = -1;
          all_pipes[i].length = 0;
          pipe_count++;
          ctx->gpr[0] = i; 
          break;
        }
      }    
      break; 
    }
    case 0x09 :{ // 0x09 => open_pipe(source_pid, dest_pid)
      int source_pid = ctx->gpr[0];
      int dest_pid = ctx->gpr[1];      
      if ((pipe_count == 0) || 
          (pcb[source_pid].status == STATUS_TERMINATED)|| 
          (pcb[dest_pid].status == STATUS_TERMINATED)) {
        ctx->gpr[0] = -1; //if there is not a pipe,or one of process is terminated
        return;
      }

      for (int i = 0; i < PIPE_MAX_NUM; i++){
        if (all_pipes[i].source_pid == source_pid && all_pipes[i].dest_pid == dest_pid) {
          ctx->gpr[0] = i; 
          return;
        }
      }   
      ctx->gpr[0] = -1; //if there is not a matching pipe
      break; 
    }
    case 0x0A :{ // 0x0A => write_pipe(pipe, x)
      int index = ctx->gpr[0];
      int x = ctx->gpr[1];
      enqueue(all_pipes[index].pipe, &all_pipes[index].tail, x, &all_pipes[index].length);    

      break; 
    }
    case 0x0B :{ // 0x0B => read_pipe(pipe)
      int index = ctx->gpr[0]; 
      if (all_pipes[index].length == 0) ctx->gpr[0] = -1;
      else ctx->gpr[0] = dequeue(all_pipes[index].pipe, &all_pipes[index].head, &all_pipes[index].length);
      break; 
    }
    case 0x0C :{ // 0x0C => close_pipe(source_pid, dest_pid)
      int source_pid = ctx->gpr[0];
      int dest_pid = ctx->gpr[1];
      if (pipe_count == 0) return;
      for (int i = 0; i < PIPE_MAX_NUM; i++){
        if (all_pipes[i].source_pid == source_pid && all_pipes[i].dest_pid == dest_pid){
          all_pipes[i].source_pid = -1;
          all_pipes[i].dest_pid = -1;
          pipe_count--;  
          break;
        }
      } 
      break; 
    }
    case 0x0D :{ // 0x0D => get_pid()
      ctx->gpr[0] = current->pid;
      break;
    }      
    case 0x0E :{ // 0x0E => change_sche(int alg_index)
      int alg_index = ctx->gpr[0];
      switch(alg_index){
        case 1: schedule_alg = &sche_round_robin; break;
        case 2: schedule_alg = &sche_priority_based; break;
        default: break;
      }
      break;
    }     
    case 0x0F :{ // 0x0F => state()
      char s0[21] = "scheduler algorithm: ";
      char r[12] = "round robin\n";
      char p[MAX_FILE_NUM] = "priority based\n";
      for (int i = 0; i < 21; i++) PL011_putc( UART1, s0[i], true );
      if (schedule_alg == &sche_round_robin) for (int i = 0; i < 12; i++) PL011_putc( UART1, r[i], true );
      else if (schedule_alg == &sche_priority_based) for (int i = 0; i < MAX_FILE_NUM; i++) PL011_putc( UART1, p[i], true );
      char s1[16] = "no. of process: ";
      for (int i = 0; i < 16; i++) PL011_putc( UART1, s1[i], true );
      if (pcb_count < 10) PL011_putc( UART1, '0' + pcb_count, true );
      else {
        PL011_putc( UART1, '0' + pcb_count / 10, true );
        PL011_putc( UART1, '0' + pcb_count % 10, true );
      }
      char s2[6] = "\npcb:[";
      for (int i = 0; i < 6; i++) PL011_putc( UART1, s2[i], true );
      for (int i = 0; i < PCB_MAX_NUM; i++){
        if (pcb[i].status != STATUS_TERMINATED){ 
          if (pcb[i].pid < 10) PL011_putc( UART1, '0' + pcb[i].pid, true );
          else {
            PL011_putc( UART1, '0' + pcb[i].pid / 10, true );
            PL011_putc( UART1, '0' + pcb[i].pid % 10, true );
          }
          PL011_putc( UART1, ',', true );
        }
      }
      PL011_putc( UART1, ']', true );
      
      char s3[6] = "\npri:[";
      for (int i = 0; i < 6; i++) PL011_putc( UART1, s3[i], true );
      for (int i = 0; i < PCB_MAX_NUM; i++){
        if (pcb[i].status != STATUS_TERMINATED){ 
          if (pcb[i].priority < 10) PL011_putc( UART1, '0' + pcb[i].priority, true );
          else {
            PL011_putc( UART1, '0' + pcb[i].priority / 10, true );
            PL011_putc( UART1, '0' + pcb[i].priority % 10, true );
          }
          PL011_putc( UART1, ',', true );
        }
      }
      PL011_putc( UART1, ']', true );
      
      PL011_putc( UART1, '\n', true );
      break;
    } 
    case 0x10 :{ // 0x10 => int disk_block_num()
      ctx->gpr[0] = disk_get_block_num();
      break;
    }  
    case 0x11 :{ // 0x10 => int disk_block_len()
      ctx->gpr[0] = disk_get_block_len();
      break;
    }
    case 0x12 :{ // 0x12 => uint32_t open(char* filename, int flag)
      char* filename = (char*) ctx->gpr[0];
      int flag = ctx->gpr[1];
      uint32_t current_directory = current->working_directory_address;
      uint32_t inode_id = findFile(filename);
      uint32_t address = findData(inode_id);
      //check if this file exit
      for (int i = 3; i < 10; i++){
        if (current->fdt[i].fcb == address && 
            current->fdt[i].flag == flag &&
            current->fdt[i].flag != -1) {
          ctx->gpr[0] = i;
          return;
        }
      }
      for (int i = 3; i < 10; i++){
        //find an empty fcb
        if (current->fdt[i].rw_pointer == -1){
          current->fdt[i].fcb = address;
          current->fdt[i].flag = flag;
          current->fdt[i].rw_pointer = 0;
          current->fdt[i].inode_id = inode_id;
          ctx->gpr[0] = i;
          break;
        }
      }
      break;
    }
    case 0x13 :{ // 0x13 => char* list_file()
//       char* filenames = (char*) malloc(16 * MAX_FILE_NUM);
//       uint8_t* data = (uint8_t*) malloc(16);
//       strcpy(filenames, "\0");
//       for (int i = 0; i < MAX_FILE_NUM; i++){   //MAX_FILE_NUM files
//         disk_rd(0x00001000 + i * 2 + 1, data, 16);
//         if (*data == 0x1) {
//           disk_rd(0x00001000 + i * 2, data, 16);
//           strcat(filenames, data);
//           strcat(filenames, "   ");
//         }
//       }
//       strcat(filenames, "\n\0");
      char* filenames = (char*) malloc(16 * MAX_FILE_NUM);
      for (int i = 0; i < MAX_FILE_NUM; i++){
        if (filename_table[i].inode_id != 0) {
          strcat(filenames, filename_table[i].filename);
          strcat(filenames, "   ");
        }
      }
      strcat(filenames,"\n");
      ctx->gpr[0] = (uint32_t) filenames;
      break;
    }
    case 0x14 :{ // 0x14 => void close(fd)
      int fd = ctx->gpr[0];
      if (2 < fd && fd < 10) {
        current->fdt[fd].fcb = 0;
        current->fdt[fd].flag = -1;
        current->fdt[fd].rw_pointer = -1;
      }
      break;
    }
    case 0x15 :{ // 0x15 => int remove_file(filename)
      char* filename = (char*) ctx->gpr[0];
      bool success = false;
      for (int i = 0; i < MAX_FILE_NUM; i++){
        if (0 == strcmp(filename_table[i].filename, filename)){
          memset(filename_table[i].filename, 0, 16);
          filename_table[i].inode_id = 0;
          success = true;
          break;
        }
      }
      if (!success) return;
      char* data = (char*) malloc(17);
      for (int i = 0; i < MAX_FILE_NUM; i++){   //MAX_FILE_NUM files
        disk_rd(0x00001000 + i * 2, data, 16);
        if (0 == strcmp(data, filename)) {
          memset(data, 0, 16);
          disk_wr(0x00001000 + i * 2, data, 16);      //clean file name
          disk_wr(0x00001000 + i * 2 + 1, data, 16);  //clean file info            
          ctx->gpr[0] = 0;
          return;
        }
      }
      ctx->gpr[0] = -1;
      break;
    }
    case 0x16 :{ // 0x15 => int word_count(fd)
      int fd = ctx->gpr[0];
      if (current->fdt[fd].rw_pointer == -1) ctx->gpr[0] = -1;
      else ctx->gpr[0] = getFileSize(fd);
      break;
    }
    default :{ // 0x?? => unknown/unsupported
      break;
    }
  }
    
  return;
}
