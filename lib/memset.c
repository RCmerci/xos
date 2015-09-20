#include "xtype.h"



/* return its 1st arg */
void* memset(void* dst, u8 c, int len)
{
  int i;
  u8 * _dst = (u8*) dst;
  for(i=0;i<len;i++) {
    _dst[i] = c;
  }
  return _dst;
}
