#include "types.h"
#include "user.h"
#include "pstat.h"

int
main(int argc, char *argv[])
{
  struct pstat ps;

  // collect information about all running processes using getpinfo system call
  if(getpinfo(&ps)){
    printf(2, "getpinfo failed\n");
    exit();
  }

  for(int i = 0; i < NPROC; i++){
    // if process is in use, print pstat info for ith process
    if(ps.inuse[i])
      printf(1, "PID: %d, tickets: %d, ticks: %d\n", ps.pid[i], ps.tickets[i], ps.ticks[i]);
  }

  exit();
}
