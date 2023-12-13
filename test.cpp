#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
  int *p = NULL;

  // 给一个NULL指令赋值，会产生 Segmentation fault 错误
  *p = 100;

  return 0;
}