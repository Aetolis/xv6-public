#include "types.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  // exit if too many arguments
  if(argc > 2){
    printf(1, "Syntax error.\n");
    exit();
  }

  // initiate a NULL pointer
  int *ptr = (void *)0;

  // if there is only one argument
  if(argc == 1){
    printf(1, "Reading NULL pointer...\n");
    // attempt to read NULL pointer
    int x = *ptr;
    printf(1 , "Null-pointer value:%p", x);
    exit();
  }

  switch (atoi(argv[1])){
    case 0:
      printf(1, "Reading NULL pointer...\n");
      // attempt to read NULL pointer
      int x = *ptr;
      printf(1 , "Null-pointer value:%p", x);
      break;

    case 1:
      printf(1, "Writing to NULL pointer...\n");
      // attempt to write to NULL pointer
      *ptr = 12;
      printf(1 , "Null-pointer value:%p", ptr);
      break;

    default:
      printf(1, "Syntax error.\n");
      break;
  }

  exit();
}
