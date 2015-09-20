#include "xglobal.h"
#include "xconst.h"
#include "common.h"

u32 seg_addr(int pid, int idx)
{
  PROCESS * proc = get_process(pid);
  DESCRIPTOR desc = proc->ldt[idx];
  return desc.base_24_31<<24 | desc.base_16_23<<16 | desc.base_0_15;
}

void * va2la(int pid, void * va)
{
  int seg_a = seg_addr(pid, INDEX_LDT_DS);
  return (void*)(seg_a + (u32)va);
}
  
