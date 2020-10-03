#include "types.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "user.h"
#include "pstat.h"

// initiate counter variables
volatile int count[3] = {0, 0, 0};

int
main(int argc, char *argv[])
{
  int pid[4];
  // allocate memory for array of pstat structs
  struct pstat *ps = malloc(50* sizeof(struct pstat));
  // set tickets for parent process
  settickets(999999);

  pid[0] = fork();
  if(pid[0] < 0){
    printf(1, "fork 1 failed\n");
  }
  // child 1
  if(pid[0] == 0){
    // set tickets to 10
    if(settickets(10) == -1){
      printf(1, "settickets failed\n");
    }
    // increment counter
    for(int i = 0; i < 20000000; i++){
      count[0]++;
    }
    printf(1, "done 1\n");
    exit();
  }

  pid[1] = fork();
  if(pid[1] < 0){
    printf(1, "fork 2 failed\n");
  }
  // child 2
  if(pid[1] == 0){
    if(settickets(20) == -1){
      printf(1, "settickets failed\n");
    }
    for(int i = 0; i < 20000000; i++){
      count[1]++;
    }
    printf(1, "done 2\n");
    exit();
  }

  pid[2] = fork();
  if(pid[2] < 0){
    printf(1, "fork 3 failed\n");
  }
  // child 3
  if(pid[2] == 0){
    if(settickets(30) == -1){
      printf(1, "settickets failed\n");
    }
    for(int i = 0; i < 20000000; i++){
      count[2]++;
    }
    printf(1, "done 3\n");
    exit();
  }

  // parent

  // check state of running processes periodically
  for(int i = 0; i < 50; i++){
    if(getpinfo(&ps[i])){
      printf(2, "getpinfo failed\n");
      exit();
    }
    sleep(13);
  }

  // wait for child processes
  wait();
  wait();
  wait();

  // print results
  for(int i = 0; i < 50; i++){
    printf(1, "-----Check %d-----\n", i+1);
    for(int j = 0; j < NPROC; j++){
      // if process is in use, print pstat info for ith process
      if(ps[i].inuse[j])
        printf(1, "PID: %d, tickets: %d, ticks: %d\n", ps[i].pid[j], ps[i].tickets[j], ps[i].ticks[j]);
    }
    printf(1, "\n");
  }

  // free dynamically allocated pstat array
  free(ps);

  exit();
}
