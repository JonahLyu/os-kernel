/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */

#include "P3.h"

void test_read_file() {

  write( STDOUT_FILENO, "<read test>\n", 12 ); 
  
  int fd = open("foo.txt", READ);
  char x[30];
  int r = read(fd, x, 16);
  if (r != 0) {
    write( STDOUT_FILENO, "read foo.txt\n", 13); 
    write( STDOUT_FILENO, x, 16); 
    write( STDOUT_FILENO, "\n", 1); 
  }
  close(fd);
  
  fd = open("bar.txt", READ);
  r = read(fd, x, 22);
  if (r != 0) {
    write( STDOUT_FILENO, "read bar.txt\n", 13); 
    write( STDOUT_FILENO, x, 22); 
    write( STDOUT_FILENO, "\n", 1); 
  }
  close(fd);
  
  fd = open("baz.txt", READ);
  r = read(fd, x, 22);
  if (r != 0) {
    write( STDOUT_FILENO, "read baz.txt\n", 13); 
    write( STDOUT_FILENO, x, 22); 
    write( STDOUT_FILENO, "\n", 1); 
  }
  close(fd);
  
  fd = open("test.txt", READ);
  r = read(fd, x, 43);
  if (r != 0) {
    write( STDOUT_FILENO, "read test.txt\n", 14); 
    write( STDOUT_FILENO, x, 43); 
    write( STDOUT_FILENO, "\n", 1); 
  }
  close(fd);
  
  fd = open("Jonah.txt", READ);
  r = read(fd, x, 124);
  if (r != 0) {
    write( STDOUT_FILENO, "read Jonah.txt\n", 15); 
    write( STDOUT_FILENO, x, 124); 
    write( STDOUT_FILENO, "\n", 1); 
  }
  close(fd);
  
  write( STDOUT_FILENO, "Finish\n", 7 ); 
  exit( EXIT_SUCCESS );
}