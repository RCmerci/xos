#include "xtype.h"
#include "xglobal.h"
#include "xconst.h"
#include "klib.h"
#include "elf.h"
#include "common.h"


void set_irq_handler(int irq, irq_handler handler)
{
  //  disable_irq(irq);
  irq_table[irq] = handler;
}

u32 get_next_pid()
{
  static u32 next_pid = -1;
  return ++next_pid;
}


void get_boot_param(struct boot_param_s * bp_p)
{
  struct boot_param_s * b = (struct boot_param_s*)BOOT_PARAM_ADDR;
  assert(b->magic_num == BOOT_PARAM_MAGIC);
  bp_p->mem_size = b->mem_size;
  bp_p->kernel_file_addr = b->kernel_file_addr;
  assert(*(u32*)bp_p->kernel_file_addr == 0x464c457f); /* 0x464c457f == ".ELF" */
}

/* 
   get kernel file 'base' and 'size'
 */
int get_kernel_map(u32 * base_p, u32 * limit_p)
{
  struct boot_param_s bp;
  get_boot_param(&bp);
  Elf32_Ehdr * e_hdr = (Elf32_Ehdr*)(bp.kernel_file_addr);
  int i;
  u32 tmp_end = 0;
  for (i = 0; i < e_hdr->e_shnum; i++) {
    Elf32_Shdr * s_hdr =  (Elf32_Shdr*)(bp.kernel_file_addr + e_hdr->e_shoff + i * e_hdr->e_shentsize);
    if (s_hdr->sh_flags & SHF_ALLOC) { /* 
				       **SHF_ALLOC ->
					  This section occupies memory during process
					  execution.  Some control sections do not
					  reside in the memory image of an object
					  file.  This attribute is off for those
					  sections.
       			      **sh_addr->
			       	  If this section appears in the memory image of a process,
		       		  this member holds the address at which the section's first
	       			  byte should reside.  Otherwise, the member contains zero.

				       */
      *base_p = s_hdr->sh_addr < *base_p ? s_hdr->sh_addr : *base_p;
      tmp_end = s_hdr->sh_addr + s_hdr->sh_size > tmp_end ?
	s_hdr->sh_addr + s_hdr->sh_size : tmp_end;
    }
  }
  assert(tmp_end > *base_p);
  *limit_p = tmp_end - *base_p - 1;
  return 0;
}
 
