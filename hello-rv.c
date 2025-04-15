#include <stdio.h>

void main()
{
  int i, a = 0;
  for(i=0; i<10; i++){
    printf("fib(%d)=%d\n",i,a);
    a += i;
  }
}

