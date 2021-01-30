# simple-operating-system

Author: Jonah Lyu

**Documentation**

For the stage 1 (b):
The priority based scheduler
The console process has a special priority: if console process does not run in three rounds of scheduling, the scheduler will dispatch to console process once.

For the stage 2 (a):
The fork child has an initial priority which is the same as the console priority.

For the stage 2 (b),
I realise the pipe IPC by creating numbers of fix empty pipes. Each pair of pid can get a unique unidirectional pipe to communicate. For the dining philosopher problem, to ensure mutual exclusion, I create a waiter process. Each philosopher's process will put down all forks and send a request to the waiter before each round. And then waiter will respond to all philosophers to tell them if they can eat. To avoid starvation, each philosopher has a priority attribute. If a philosopher does not eat in last round, the priority will increase. And the waiter will serve the philosopher with the highest priority first.

For the stage 3,
The file system support a simple inode-based structure with 65536 blocks and 16 bytes block length:

0x1000 - 0x1FFF file name blocks:
```
    Each file takes two blocks, the first blocks stores the file name string data, the second block stores file info:
    Byte 0: a flag to indicate if this file exists
    Byte 1-2: the inode id
```   
0x2000 - 0x3FFF inode blocks:
```
    Each inode takes one block:
    Byte 0: a flag to indicate if this inode is used
    Byte 1-2: the file size
    Byte 3-15: the data block pointer
```
0x4000 - 0xFFFF data blocks

The current implementation does not support the file hierarchy. All operations(open, close, read, write) are running in one root directory and  all files are stored in continuous block. Each process has file descriptor table to trace the file state. A file name table is used for quick file search. Meanwhile, I added some useful commands and user programs to run in console:
```
	ls         #to list all files in current directory
	rm <file>  #to remove a file in current directory
        state      #show the current system state:number of processes, scheduler algorithm ,process pid
	cs 1        #change scheduler algorithm to round robin
        cs 2        #change scheduler algorithm to priority base
        execute test_write      #test open, close and write functions, several files will be written or appended into root directory
	execute test_read       #test open, close and read functions, several files will be read and print to UART0 terminal
	execute test_wc         #test finding the number of byte and lines of a file.
	disk       #show the disk info on shell
	nice <pid> <priority>   #set priority of a process
```
