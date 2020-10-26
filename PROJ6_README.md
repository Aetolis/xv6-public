# CS372 Project 6: XV6 Virtual Memory

## Null-pointer Dereference
The following files were edited:
  1. exec.c
     * Change `sz` variable on line 43 to 4096 instead of 0 so that when `exec` is called, programs are loaded onto the second page instead of the first.
  2. vm.c
     * In function `copyuvm`, change for loop on line 326 to start at 4096 instead of 0 so that the copy of the parent process is loaded onto the second page instead of the first. This ensures that `fork()` operates as expected.
  3. Makefile
     * Change entry point of user programs (line 150) when being compiled to start on the second page (0x1000) instead of the first.
  4. syscall.c
     * Made changes to `fetchint` and `fetchstr` to check if address being dereferenced is legal. Avoid checking for first process, init, because we are only interested in user processes.

To test implementation, use user program `nulltest` from `nulltest.c`. Call `nulltest` or `nulltest 0` to attempt to read a null pointer and `nulltest 1` to attempt to write to a null pointer.

## Read-only Code
The two new system calls, `mprotect` and `munprotect` are implemented

The user program, `mprottest`, tests the functionality of `mprotect` and `munprotect` system calls, and is implemented in `mprottest.c`. The `sbrk` system call is used to grow the memory of the current process by one page.
