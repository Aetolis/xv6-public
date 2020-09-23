#include "types.h"
#include "user.h"
#include "date.h"

int
main(int argc, char *argv[])
{
  struct rtcdate r;

  // pass in address of rtcdate struct to date syscall
  if (date(&r)) {
    printf(2, "date failed\n");
    exit();
  }

  // your code to print the time in any format you like...
  printf(1, "%d:%d:%d\n", r.hour, r.minute, r.second);

  exit();
}
