#include "xtype.h"

/*
  gen_byte(2,5) -> 00111100
 */
u8 gen_byte(int start, int end)
{
  u8 rst = 0;
  int i;
  for (i=0;i<end+1;i++) {
    rst += (1 << i);
  }
  for (i=0;i<start;i++) {
    rst -= (1 << i);
  }
  return rst;
}
