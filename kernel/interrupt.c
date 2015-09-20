#include "xtype.h"
#include "xglobal.h"

void exception_handler(u8 vector, u32 error_code, u32 eip, u32 cs, u32 eflags)
{
  	static char * err_msg[] = {"#DE Divide Error",
			    "#DB RESERVED",
			    "--  NMI Interrupt",
			    "#BP Breakpoint",
			    "#OF Overflow",
			    "#BR BOUND Range Exceeded",
			    "#UD Invalid Opcode (Undefined Opcode)",
			    "#NM Device Not Available (No Math Coprocessor)",
			    "#DF Double Fault",
			    "    Coprocessor Segment Overrun (reserved)",
			    "#TS Invalid TSS",
			    "#NP Segment Not Present",
			    "#SS Stack-Segment Fault",
			    "#GP General Protection",
			    "#PF Page Fault",
			    "--  (Intel reserved. Do not use.)",
			    "#MF x87 FPU Floating-Point Error (Math Fault)",
			    "#AC Alignment Check",
			    "#MC Machine Check",
			    "#XF SIMD Floating-Point Exception"
	};

	int text_color = GREY_B | RED_C;
	int content_color = BLACK_B | YELLOW_C;
	disp_position = 0;
	xdisp_color_str("Exception:", text_color);
	xdisp_color_str(err_msg[vector], content_color);
	xdisp_color_str("\n", text_color);
	if (error_code != 0xFFFFFFFF){
	  xdisp_color_str("Error Code:", text_color);
	  xdisp_u32(error_code);
	}
	xdisp_color_str("eflags:", text_color);
	xdisp_u32(eflags);
	xdisp_color_str("cs:", text_color);
	xdisp_u32(cs);
	xdisp_color_str("eip:", text_color);
	xdisp_u32(eip);
}

void spurious_irq(u32 irq)
{
  xdisp_color_str("irq", GREEN_C|BLACK_B);
  xdisp_u32(irq);
  xdisp_color_str("\n", 0x00);
}
