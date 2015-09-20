#include "xglobal.h"
#include "xconst.h"
#include "xtype.h"
#include "klib.h"

/* 中断 们 */
void divide_error();
void single_step_exception();
void nmi();
void breakpoint_exception();
void overflow();
void bounds_check();
void inval_opcode();
void copr_not_available();
void double_fault();
void copr_seg_overrun();
void inval_tss();
void segment_not_present();
void stack_exception();
void general_protection();
void page_fault();
void copr_error();

void hwint00();
void hwint01();
void hwint02();
void hwint03();
void hwint04();
void hwint05();
void hwint06();
void hwint07();
void hwint08();
void hwint09();
void hwint10();
void hwint11();
void hwint12();
void hwint13();
void hwint14();
void hwint15();

void sys_call();

#define vir2phy(seg, offset) (u32)((u32)seg + (u32)offset)

static u32 seg2phy(u16 seg)
{
  DESCRIPTOR * desc_p = &gdt[seg>>3];
  return desc_p->base_0_15 | (desc_p->base_16_23 << 16) | (desc_p->base_24_31 << 24);
}


static void init_8259A()
{
  xout_byte(INT_M_CTL, 0x11);
  xout_byte(INT_S_CTL, 0x11);
  xout_byte(INT_M_CTLMASK, 0x20);
  xout_byte(INT_S_CTLMASK, 0x28);
  xout_byte(INT_M_CTLMASK, 0x04);
  xout_byte(INT_S_CTLMASK, 0x02);
  xout_byte(INT_M_CTLMASK, 0x01);
  xout_byte(INT_S_CTLMASK, 0x01);
  xout_byte(INT_M_CTLMASK, 0xF8); // B:硬盘， 8:clock, keyboard, 主从线
  xout_byte(INT_S_CTLMASK, 0xBF);
  
}

static void init_idt_gate(u8 vector, u32_handler handler, u8 type, u8 privilege)
{
  GATE * gate_p = &idt[vector];
  gate_p->offset_0_15 = (u16)((u32)handler & 0xFFFF);
  gate_p->offset_16_31 = (u16)(((u32)handler >> 16) & 0xFFFF);
  gate_p->selector = SELECTOR_KERNEL_CS;
  gate_p->param_count = 0;
  gate_p->attr = type | (privilege << 5);
}

void init_descriptor(DESCRIPTOR * desc_p, u32 base, u32 limit, u16 attr)
{
  desc_p->limit_0_15 = (u16)(limit & 0xFFFF);
  desc_p->base_0_15 = (u16)(base & 0xFFFF);
  desc_p->base_16_23 = (u8)((base>>16)&0xFF);
  desc_p->base_24_31 = (u8)((base>>24)&0xFF);
  desc_p->attr1 = (attr & 0xFF);
  desc_p->attr2 = ((attr>>8) & 0xF0) | ((limit>>16) & 0x0F);
}
/* -------------------------------------------------------------------------------------- */

/* 把gdt从loader复制到kernel， 并加上ldt的描述符 和 tss*/
void init_gdt()
{
  xmemcpy(&gdt, *(u32*)(&gdt_ptr[2]), *(u16*)(&gdt_ptr[0])+1);

  u16 * gdt_limit = (u16*)&gdt_ptr[0];
  u32 * gdt_base  = (u32*)&gdt_ptr[2];
  *gdt_limit = GDT_DESC_NUM * sizeof(DESCRIPTOR) - 1;
  *gdt_base  = (u32)&gdt;

  init_descriptor(gdt+INDEX_TSS,
  		  vir2phy(seg2phy(SELECTOR_KERNEL_DS), &tss),
  		  sizeof(tss) - 1,
  		  DA_TSS);
  int i=0;
  for (;i<LDT_NUM;i++){
    init_descriptor(gdt+INDEX_FIRST_LDT+i,
  		    vir2phy(seg2phy(SELECTOR_KERNEL_DS), process_table[i].ldt),
  		    sizeof(DESCRIPTOR)*LDT_DESC_NUM - 1,
  		    DA_LDT);
  }
}


void init_tss()
{
  tss.ss0 = SELECTOR_KERNEL_DS;
  tss.iobase = sizeof(tss);
}


void init_idt()
{
  u16 * idt_limit = (u16*)&idt_ptr[0];
  u32 * idt_base  = (u32*)&idt_ptr[2];
  *idt_limit = IDT_GATE_NUM * sizeof(GATE) - 1;
  *idt_base  = (u32)&idt;
}

void init_process()
{
  /* int i = 0; */
  /* for(;i<PROCESS_NUM;i++){ */
  /*   PROCESS * p = &(process_table[i]); */
  /*   p->ldt_selector = (INDEX_FIRST_LDT+i) * 8; */
  /*   xmemcpy(&p->ldt[INDEX_LDT_CS], &gdt[INDEX_KERNEL_CS], sizeof(DESCRIPTOR)); */
  /*   p->ldt[INDEX_LDT_CS].attr1 = DA_C | PRIVILEGE_USER<<5; */
  /*   xmemcpy(&p->ldt[INDEX_LDT_DS], &gdt[INDEX_KERNEL_DS], sizeof(DESCRIPTOR)); */
  /*   p->ldt[INDEX_LDT_DS].attr1 = DA_DRW | PRIVILEGE_USER<<5; */

  /*   p->stack_frame.gs = SELECTOR_KERNEL_VIDEO | PRIVILEGE_KRNL; */
  /*   p->stack_frame.fs = 8 | SA_TI | PRIVILEGE_USER; */
  /*   p->stack_frame.es = 8 | SA_TI | PRIVILEGE_USER; */
  /*   p->stack_frame.ds = 8 | SA_TI | PRIVILEGE_USER; */
  /*   p->stack_frame.cs = 0 | SA_TI | PRIVILEGE_USER; */
  /*   p->stack_frame.ss = 8 | SA_TI | PRIVILEGE_USER; */
  /*   p->stack_frame.eip = (u32)task_table[i].task_entry; */
  /*   p->stack_frame.esp = (u32)&(task_stack[TASK_STACK_SIZE*(i+1)]); // 栈自高向低长 */
  /*   p->stack_frame.eflags = 0x3202; */

  /*   p->pid = i; */
  /*   xmemcpy(p->name, task_table[i].name, 50); */
  /*   p->nice = task_table[i].nice; */
  /*   p->ticks = p->nice; */
  /*   p->tty_num = task_table[i].tty_num; */

  /*   p->status = STAT_PAUSE; */
  /*   p->msg_count = 0; */
  /*   p->head_p = p->recv_queue; */
  /*   p->tail_p = p->recv_queue; */

  /*   p->int_msg_count = 0; */
  /*   p->int_head_p = p->int_recv_queue; */
  /*   p->int_tail_p = p->int_recv_queue; */

  /*   memset(&(p->open_files), 0, sizeof(OPEN_FILE)*MAX_FILE_OPEN_PER_PROC); */

  /*   p->ext_flag = 1; */
  /* } */
  int i;
  
  int ext_flag = 0;
  u16 ldt_selector = 0;
  u32 privilege = 0;
  u32 rpl       = 0;
  u32 eflags    = 0;
  
  for (i = 0; i < PROCESS_NUM; i++) {
    PROCESS * p = &(process_table[i]);
    if (i > TASK_NUM - 1) {	/* 不是最初就有进程, 略过 */
      p->ext_flag = 0;
      continue;
    } else if (i < NR_SYS_TASKS) { /* 是系统进程 */
      ext_flag = 1;
      ldt_selector = (INDEX_FIRST_LDT + i) << 3;
      privilege = PRIVILEGE_TASK;
      rpl = PRIVILEGE_TASK;
      eflags = 0x1202;
    } else {			/* 最初就有的一般进程 */
      ext_flag = 1;
      ldt_selector = (INDEX_FIRST_LDT + i) << 3;
      privilege = PRIVILEGE_USER;
      rpl = PRIVILEGE_USER;
      eflags = 0x202;
    }
    p->ldt_selector = ldt_selector;
    p->ext_flag = ext_flag;
    
    STACKFRAME * sf_p = &(p->stack_frame);
    sf_p->gs = SELECTOR_KERNEL_VIDEO | rpl;
    sf_p->fs = SELECTOR_LDT_DS | SA_TI | rpl;
    sf_p->es = SELECTOR_LDT_DS | SA_TI | rpl;
    sf_p->ds = SELECTOR_LDT_DS | SA_TI | rpl;
    sf_p->cs = SELECTOR_LDT_CS | SA_TI | rpl;
    sf_p->ss = SELECTOR_LDT_DS | SA_TI | rpl;
    sf_p->eflags = eflags;

    sf_p->eip = (u32)task_table[i].task_entry;
    sf_p->esp = (u32)&(task_stack[TASK_STACK_SIZE*(i+1)]); /*  栈自高向低长 */

    p->pid = get_next_pid();
    xmemcpy(p->name, task_table[i].name, 50);
    p->nice = task_table[i].nice;
    p->ticks = p->nice;
    p->tty_num = task_table[i].tty_num;
    p->status = STAT_PAUSE;
    p->send_pid = NO_PID;
    p->msg_count = 0;
    p->head_p = p->recv_queue;
    p->tail_p = p->recv_queue;
    memset(&(p->open_files), 0, sizeof(OPEN_FILE)*MAX_FILE_OPEN_PER_PROC);
    p->parent_pid = NO_PID;
    p->ext_flag = 1;

    if (! strcmp(p->name, "INIT")) { /* proc is INIT */
      u32 base, limit;
      get_kernel_map(&base, &limit); /* 得到kernel.bin 在内存中的位置及大小，limit=size-1 */
      init_descriptor(&(p->ldt[INDEX_LDT_CS]),                              /* init 进程 */
		      0,	                                 /* base:0                       */
		      (1 + (base + limit) >> LIMIT_4K_SHIFT),     /* limit: kernel.bin's end addr */
		      DA_LIMIT_4K | DA_32 | DA_C | privilege << 5);
      init_descriptor(&(p->ldt[INDEX_LDT_DS]),
		      0,
		      (1 + (base + limit) >> LIMIT_4K_SHIFT),
		      DA_LIMIT_4K | DA_32 | DA_DRW | privilege << 5);
    } else {
      p->ldt[INDEX_LDT_CS] = gdt[INDEX_KERNEL_CS];
      p->ldt[INDEX_LDT_DS] = gdt[INDEX_KERNEL_DS];
      p->ldt[INDEX_LDT_CS].attr1 = DA_C | privilege << 5;
      p->ldt[INDEX_LDT_DS].attr1 = DA_DRW | privilege << 5;
    }
  }
}


void init_interrupt()
{
  init_8259A();
  init_idt_gate(0, divide_error, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(1, single_step_exception, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(2, nmi, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(3, breakpoint_exception, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(4, overflow, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(5, bounds_check, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(6, inval_opcode, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(7, copr_not_available, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(8, double_fault, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(9, copr_seg_overrun, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(10, inval_tss, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(11, segment_not_present, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(12, stack_exception, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(13, general_protection, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(14, page_fault, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(16, copr_error, DA_IGATE, PRIVILEGE_KRNL);
  
  //
  init_idt_gate(IRQ0_8259+0, hwint00, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(IRQ0_8259+1, hwint01, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(IRQ0_8259+2, hwint02, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(IRQ0_8259+3, hwint03, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(IRQ0_8259+4, hwint04, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(IRQ0_8259+5, hwint05, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(IRQ0_8259+6, hwint06, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(IRQ0_8259+7, hwint07, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(IRQ8_8259+0, hwint08, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(IRQ8_8259+1, hwint09, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(IRQ8_8259+2, hwint10, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(IRQ8_8259+3, hwint11, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(IRQ8_8259+4, hwint12, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(IRQ8_8259+5, hwint13, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(IRQ8_8259+6, hwint14, DA_IGATE, PRIVILEGE_KRNL);
  init_idt_gate(IRQ8_8259+7, hwint15, DA_IGATE, PRIVILEGE_KRNL);

  init_idt_gate(SYSCALL_INT_VECTOR, sys_call, DA_IGATE, PRIVILEGE_USER);
}

void init_pit()
{
  xout_byte(TIMER_MODE, RATE_GENERATOR);
  xout_byte(TIMER0, (u8)(TIMER_FREQ / HZ));
  xout_byte(TIMER0, (u8)((TIMER_FREQ / HZ) >> 8));
}
