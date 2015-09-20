#ifndef _KERNEL_KLIB
#define _KERNEL_KLIB

#include "xtype.h"

void set_irq_handler(int irq, irq_handler handler);

u32 get_next_pid();		/* get next pid，用于fork时指定pid */


/* 给 get_boot_param 用的 */
struct boot_param_s {
  u32 magic_num;
  u32 mem_size;
  u32 kernel_file_addr;
};

void get_boot_param(struct boot_param_s * bp_p);

int get_kernel_map(u32 * base_p, u32 * limit_p);

void init_descriptor(DESCRIPTOR * desc_p, u32 base, u32 limit, u16 attr);

u16 get_ldtr();

#endif // _KERNEL_KLIB
