#include "types.h"
#include "user.h"

// prevent GCC from optimizing the recursive function
// https://stackoverflow.com/questions/2219829/how-to-prevent-gcc-optimizing-some-statements-in-c
#pragma GCC push_options
#pragma GCC optimize ("O0")
static int recursive_test(int n)
{
  if(n == 0)
    return n;
  return recursive_test(n-1);
}
#pragma GCC pop_options

int main(int argc, char *argv[])
{
  // check if second argument is specified
  if(argc != 2){
    printf(1, "Usage: stacktest <recursive levels>\n");
    exit();
  }

  // check if program should be testing heap
  if(strcmp(argv[1], "-h") == 0){
    // this function call should result in an error
    // theoretically 534379*4096 is the largest value sbrk can take
    sbrk(524280*4096);
  }

  else{
    int n = atoi(argv[1]);

    printf(1, "Initial value of n: %d\n", n);
    printf(1, "Result: %d\n", recursive_test(n));
  }
  exit();
}
