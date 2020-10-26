#include "types.h"
#include "user.h"
#include "mmu.h"

int
main(int argc, char *argv[])
{
  int pid;
  // allocate x as end address of current process memory
  char *x = sbrk(0);
  printf(1, "sbrk(0): 0x%p\n", x);

  // grow memory of process by 1 page
  sbrk(PGSIZE);
  printf(1, "sbrk(0) after calling sbrk(PGSIZE): 0x%p\n", sbrk(0));

  // initialize x as 10
  *x = 10;
  // protect 1 page of mem
  mprotect(x, 1);

  if((pid = fork()) < 0)
    printf(1, "fork failed\n");

  // child
  if(pid == 0){
    // read protected value
    printf(1, "Dereference x: %d\n", *x);
    // remove protection
    printf(1, "Removing protection from x.\n");
    munprotect(x, 1) ;
    // write to x
    *x = 5;
    printf(1, "x = %d\n",*x);
    exit();
  }

  // parent
  wait();
  // protection is not removed so dereferencing x should result in trap
  printf(1, "You just activated my trap card!\n");
  *x = 5;

  exit();
}
