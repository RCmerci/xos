#include "xtype.h"
#include "xglobal.h"
#include "xconst.h"
#include "common.h"

u8 tool_buf[200];

typedef struct stat_name_s{
  char name [MAX_FILENAME_LEN];
  u8 stat;
  int ticks;
}STAT_NAME;
void get_proc_stat_name ()
{
  int i;
  STAT_NAME * buf = (STAT_NAME*)tool_buf;
  for (i = 0; i < PROCESS_NUM; i++) {
    if (process_table[i].ext_flag) {
      strcpy(process_table[i].name, buf->name);
      buf->stat = process_table[i].status;
      buf->ticks = process_table[i].ticks;
      buf++;
    }
  }
}
