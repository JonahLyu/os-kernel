/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */

#include "P3.h"

void test_wc() {

  write( STDOUT_FILENO, "<word count test>\n", 18 ); 
  char wc[5] = ""; 
  char lc[5] = ""; 
  int fd = open("Jonah.txt", READ);
  int count = word_count(fd);
  if (count != -1) {
    //print word count
    write( STDOUT_FILENO, "Jonah.txt\nword count: ", 22 );
    itoa(wc, count);
    write( STDOUT_FILENO, wc, 3 );
    write( STDOUT_FILENO, "\n", 1 );
    //line count
    char data[count + 1];
    read(fd, data, count);
    int lines = 0;
    for (int i = 0; i < count; i++){
      if (data[i] == '\n') lines = lines + 1;
    }
    //print line count
    write( STDOUT_FILENO, "Number of lines: ", 17);
    itoa(lc, lines);
    write( STDOUT_FILENO, lc, 3 );
    write( STDOUT_FILENO, "\n", 1 );
  }
  else write( STDOUT_FILENO, "Fail", 4 );
  
  close(fd);

  
  write( STDOUT_FILENO, "Finish\n", 7 ); 
  exit( EXIT_SUCCESS );
}