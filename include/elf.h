#ifndef _KERNEL_ELF
#define _KERNEL_ELF
#include "xtype.h"

#define EI_NIDENT 16

typedef struct {
  u8       e_ident[EI_NIDENT];
  u16      e_type;
  u16      e_machine;
  u32      e_version;
  u32      e_entry;
  u32      e_phoff;
  u32      e_shoff;
  u32      e_flags;
  u16      e_ehsize;
  u16      e_phentsize;
  u16      e_phnum;
  u16      e_shentsize;
  u16      e_shnum;
  u16      e_shstrndx;
} Elf32_Ehdr;

typedef struct {
  u32   p_type;
  u32   p_offset;
  u32   p_vaddr;
  u32   p_paddr;
  u32   p_filesz;
  u32   p_memsz;
  u32   p_flags;
  u32   p_align;
} Elf32_Phdr;

typedef struct {
  u32   sh_name;
  u32   sh_type;
  u32   sh_flags;
  u32   sh_addr;
  u32   sh_offset;
  u32   sh_size;
  u32   sh_link;
  u32   sh_info;
  u32   sh_addralign;
  u32   sh_entsize;
} Elf32_Shdr;

/* ------------------------------------------------------------------------------------ */
/*
  section header , program header 的 FLAGs.
 copy from http://www.tsri.com/jeneral/kernel/include/linux/elf.h/source/SOURCE-elf.h.html
*/
/* ------------------------------------------------------------------------------------- */

/* sh_type */
#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_HASH 5
#define SHT_DYNAMIC 6
#define SHT_NOTE 7
#define SHT_NOBITS 8
#define SHT_REL 9
#define SHT_SHLIB 10
#define SHT_DYNSYM 11
#define SHT_NUM 12
#define SHT_LOPROC 0x70000000
#define SHT_HIPROC 0x7fffffff
#define SHT_LOUSER 0x80000000
#define SHT_HIUSER 0xffffffff

/* sh_flags */
#define SHF_WRITE 0x1
#define SHF_ALLOC 0x2
#define SHF_EXECINSTR 0x4
#define SHF_MASKPROC 0xf0000000

/* special section indexes */
#define SHN_UNDEF 0
#define SHN_LORESERVE 0xff00
#define SHN_LOPROC 0xff00
#define SHN_HIPROC 0xff1f
#define SHN_ABS 0xfff1
#define SHN_COMMON 0xfff2
#define SHN_HIRESERVE 0xffff
 
#endif // _KERNEL_ELF
