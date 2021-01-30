/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */

#include "P3.h"

void test_write_file() {

  write( STDOUT_FILENO, "<write test>\n", 13 ); 
  
  int fd = open("foo.txt", WRITE);
  int r = write(fd, "computer science", 16);
  if (r != 0) write( STDOUT_FILENO, "write foo.txt\n", 14 ); 
  close(fd);
  
  fd = open("bar.txt", WRITE);
  r = write(fd, "hello, this is a test!", 22);
  if (r != 0) write( STDOUT_FILENO, "write bar.txt\n", 14 ); 
  close(fd);
  
  fd = open("baz.txt", WRITE);
  r = write(fd, "University of Bristol.", 22);
  if (r != 0) write( STDOUT_FILENO, "write baz.txt\n", 14 ); 
  close(fd);
  
  fd = open("test.txt", WRITE);
  r = write(fd, "Concurrent programming deadline is April 5.", 43);
  if (r != 0) write( STDOUT_FILENO, "write test.txt\n", 15 ); 
  close(fd);
  
  //test append
  fd = open("Jonah.txt", WRITE);
  r = write(fd, "Jonah's file system.\n", 21);
  if (r != 0) write( STDOUT_FILENO, "append Jonah.txt\n", 17 ); 
  r = write(fd, "This just a test.\n", 18);
  if (r != 0) write( STDOUT_FILENO, "append Jonah.txt\n", 17 ); 
  r = write(fd, "There is a computer on the table.\n", 34);
  if (r != 0) write( STDOUT_FILENO, "append Jonah.txt\n", 17 ); 
  r = write(fd, "Please do not stack chairs in front of fire exits.\n", 51);
  if (r != 0) write( STDOUT_FILENO, "append Jonah.txt\n", 17 ); 
  close(fd);
  
  write( STDOUT_FILENO, "Finish\n", 8 ); 
  exit( EXIT_SUCCESS );
}