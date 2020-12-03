#  CS-372 Final Project: `xv6` Process Address Space

## Address Space Re-arragement

### Null Pointer Dereference

The changes that were made to implement a null pointer protected page is very similar to the changes made for project 6.

Code from the following files were changed:
  1. exec.c
     * `exec` loads programs starting from second page instead of first.
  2. vm.c
     * `copyuvm` loads copy of parent process onto second page instead of first.
  3. Makefile
     * Change entry point of user programs when being compiled to start on the second page (0x1000) instead of the first.

### Revised Address Space
The address space of user processes were changed so that the stack grows down starting at the top of the address space, 0x7FFFFFFF (KERNBASE-1), and the heap, which grows up, is placed just above the code and the global variables. This allows both dynamic regions to grow until they potentially collide.

The following files were changed to implement this revised address space:
  1. proc.h
    * Added the variable, uint stack_sz, on line 52 of `struct proc`. This variable keeps track of the number of pages allocated or size of the new stack.
  2. exec.c
    * After `exec` allocates a new page table with `setupkvm` and allocates space and loads each segment of the ELF file using `allocuvm` and `loaduvm`, we now create the stack according to the specifications of the revised address space starting at line 65 using 'allocuvm'. `allocuvm` creates the new stack by taking the page table as its first parameter, the address of the stack page we are allocating as its second parameter, and an address value that is slightly larger than the address of the first stack page as the last parameter. We are only allocating one page so as long as the last parameter is slightly larger (add value that is less than PGSIZE) than the address used as the second parameter, `allocuvm` will allocate one page.
    * In addition, instead of allocating the stack pointer to be sz, the stack pointer now points to KERNBASE-1, which is the bottom of our stack.
    * On line 104, set the stack_sz variable to 1 as this is the initial size of our stack.
  3. vm.c
    * Changes have to be made to `copyuvm` in order to account for the new location of the stack.
    * Add an additional parameter, uint stack_sz, to `copyuvm`.
    * After copying the pages up to uint sz, we use a similar for loop to instead iterate through and copy the pages of the new stack as well.
  4. syscall.c
     * Changes made to functions, `fetchint`, `fetchstr`, and `argptr` to ensure the the address being passed to these functions are actually on the new stack.

### Automatically Growing Stack

To automatically grow the stack when needed, we have to make changes to xv6's existing trap handler implemented in trap.c. By adding a case for page faults (T_PGFLT) on line 81, we can check if the page fault occured on the page right below the top of the stack and if the condition that there should always be 5 unallocated pages between the stack and heap is true. We then increase the size of the stack by one if needed using `allocuvm`. We can identify the address that resulted in a page fault by looking in the CR2 control register which records the page fault linear address in the event of a page fault. If `allocuvm` fails or if the page fault occurred more than a page below the top of the stack, the process is killed.

We also have to make changes to the system call `sys_sbrk` in sysproc.c, line 46, to ensure that we are checking if the condition that there is a 5 page gap between the stack and heap is true when the size of the heap is being increased.

### Testing

To test the implementation of the null pointer protected page, use user program `nulltest` from `nulltest.c`. Call `nulltest` or `nulltest 0` to attempt to read a null pointer and `nulltest 1` to attempt to write to a null pointer.

The revised process address space can be tested using the user program `stacktest` implemented in `stacktest.c`. Specify the number of recursive iterations like so: `stacktest 100000`. A larger number means more recursive iterations. Use `stacktest -h` to test the maintenence of the 5 gap pages when automatically growing heap. This function call should result in an error.
