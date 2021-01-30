/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */

#include "P3.h"


void main_P6() {
  
  write( STDOUT_FILENO, "<P6>", 4 ); 
  
  create_pipe(1,2);
  create_pipe(2,1);
  while(1){
    int index = open_pipe(1,2);
    if (index >= 0) break;
  }
  
//   create_pipe(16,17);
//   create_pipe(17,16);
  
  for (int i = 0; i < 10; i++){
    int r1 = -1;
    write( STDOUT_FILENO, "<P6 write>", 10 ); 
    int index = open_pipe(1,2);
    if (index < 0) {
      write( STDOUT_FILENO, " terminate ", 11 ); 
      exit( EXIT_SUCCESS );
    }
    write_pipe(index,i);
    while(r1 == -1){
      index = open_pipe(2,1);
      if (index < 0) break;
      r1 = read_pipe(index);
    }

  }
    
  write( STDOUT_FILENO, " Finish ", 8 ); 

  
    
  exit( EXIT_SUCCESS );
}