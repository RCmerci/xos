#include "xconst.h"


static void spin()
{
  while(1);
}

void assert_fail(char * exp, char * file, char * base_file, int line)
{
  printk(ASSERT_MAGIC, "Assert:%s, file:%s, base_file:%s, line:%d\n", exp, file, base_file, line);
  spin();
}


