# CS372 Project 4: Intro to Kernel Hacking

## System Call Tracing
System call tracing is on by default. To toggle system call tracing off, comment out line 13 in syscall.c and line 20 in sysfile.c. This was implemented using preprocessor directives, `#ifdef` and `#endif`.

Also attempted the challenge to print system call arguments. Syntax is usually in the form of:
> write -> 1\t(1, i, 1)

Except for the exec system call, which is in the form:
> path: /init exec -> 0 (2ff0)

## Date System Call
In order to implement the "date" system call in xv6, I had to modify anf create the following files:
1. date.c
  - user-level program that calls the date system call. UTC time is printed out in the format of `hours:minutes:seconds`.
2. Makefile
  - added `_date` to `UPROGS` definition in Makefile so that it is possible to run date user program from xv6 shell.
3. syscall.c
  - included function declaration of `sys_date` system call which is implemented in sysproc.c on line 111. Also added `sys_date` to the array of pointers to system calls on line 135. So when a user program calls system call 22, the array here will point us to the corresponding system call function.
4. syscall.h
  - syscall.h defines the numbers associated with every xv6 system call. Added `SYS_date` as system call number 22.
5. sysproc.c
  - this file is where the date system call is implemented starting on line 94. Implementation is very simple using the `cmostime()` helper function to save the UTC real time clock value in the `struct rtcdate` passed as a pointer.
6. user.h
  - added date system call to declaration of system call functions in user.h.
7. usys.S
  - usys.S is a assembly language file for the C preprocessor to process. This file allows the kernel to use the value in `%eax` to vector to the system call that is being invoked. Added date system call to line 32.

My strategy for knowing which files to modify to create the date system call was to use `grep -n uptime *.[chS]`.
