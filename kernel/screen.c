#include "xconst.h"
#include "xtype.h"
#include "xglobal.h"
static CONSOLE * current_console;

void clear_screen(CONSOLE * console)
{
  int i = 0;
  for (;i<80*25;i++){
    xdisp_word_at(0x0f00|' ', console->current_screen_addr+i*2);
  }
}

/* 这个 pos 是没有除过 2 的数字 （每个字符占了2字节） */
void set_cursor(u32 pos)
{
  xout_byte(CRT_ADDR, CRT_DATA_CURSOR_LOCATION_HIGH);
  xout_byte(CRT_DATA, ((pos/2)>>8) & 0xFF);
  xout_byte(CRT_ADDR, CRT_DATA_CURSOR_LOCATION_LOW);
  xout_byte(CRT_DATA, (pos/2) & 0xFF);
}


/*
`line' 必须大于 0 
 @return:
   0 : 正常
   1 : line 参数不对
   2 : direction 参数不对
   3 : 滚不了了，已经到头了

 */
int scroll(CONSOLE * console, u32 direction, u32 line)
{

  if (line <= 0) {
    return 1;
  }
  u32 next_addr, rst;
  if (direction == SCROLL_UP) { /* up */
    next_addr = console->current_screen_addr - 80 * line * 2;
    if (next_addr >= console->start_addr &&
	next_addr <= console->end_addr) {
      console->current_screen_addr = next_addr;
      rst = 0;
    }
    else {
      console->current_screen_addr = console->start_addr;
      next_addr = console->start_addr;
      rst = 3;
    }
  }
  else if (direction == SCROLL_DOWN) { /* down */
    next_addr = console->current_screen_addr + 80 * line * 2;
    if (next_addr >= console->start_addr &&
	next_addr <= console->end_addr) {
      console->current_screen_addr = next_addr;
      rst = 0;
    }
    else {
      console->current_screen_addr = console->end_addr;
      next_addr = console->end_addr;
      rst = 3;
    }
  }
  else { 			/* 错误 'direction' 参数 */
    return 2;
  }
  disable_int();

  xout_byte(CRT_ADDR, CRT_DATA_START_ADDR_HIGH);
  xout_byte(CRT_DATA, ((next_addr/2)>>8) & 0xFF);
  xout_byte(CRT_ADDR, CRT_DATA_START_ADDR_LOW);
  xout_byte(CRT_DATA, (next_addr/2) & 0xFF);
      
  enable_int();
  return rst;
}


/* 0 : 正常
   -1 : 错误
*/
int switch_console(int num)
{
  if (num < 0 || num >= CONSOLE_NUM) {
    return -1;
  }
  u32 start_addr = console_table[num].start_addr;
  disable_int();

  xout_byte(CRT_ADDR, CRT_DATA_START_ADDR_HIGH);
  xout_byte(CRT_DATA, ((start_addr/2)>>8) & 0xFF);
  xout_byte(CRT_ADDR, CRT_DATA_START_ADDR_LOW);
  xout_byte(CRT_DATA, (start_addr/2) & 0xFF);
  
  enable_int();
  return 0;
}
