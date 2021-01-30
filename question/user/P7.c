/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */

#include "P3.h"

void main_P7() {

  write( STDOUT_FILENO, "<P7> ", 4 ); 
  
  
  for (int i = 0; i < 10; i++){
    int r1 = -1;
    while(r1 == -1){
      int index = open_pipe(1,2);
      if (index < 0) {
        write( STDOUT_FILENO, " terminate ", 11 ); 
        exit( EXIT_SUCCESS );
      }
      r1 = read_pipe(index);    
    }

    char a = '0' + r1;
    char s[1] = {a};
    write(STDOUT_FILENO, s, 1);
    write_pipe(open_pipe(2,1), 1);
  }
  
  
 
  
  
  
  

  
  write( STDOUT_FILENO, " Finish ", 8 ); 
  exit( EXIT_SUCCESS );
}