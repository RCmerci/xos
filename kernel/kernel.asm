%include "xconst.inc"	
	;; 导入
	extern init_gdt
	extern init_idt
	extern exception_handler
	extern init_interrupt
	extern spurious_irq
	extern init_tss
	extern init_process_ldt
	extern kmain
	extern irq_schedule
	extern init_pit
	;; =================
	extern xdisp_color_str
	extern xdisp_u32
	extern xdisp_al
	extern delay
	extern xin_byte
	extern init_keyboard
	
	;;
	extern _get_ticks
	extern tickso
	extern ticks
	;; 全局变量
	extern gdt_ptr
	extern gdt
	extern idt_ptr
	extern disp_position
	extern ready_process_p
	extern tss
	extern process_table
	extern task_table
	extern reenter
	extern irq_table
	extern syscall_table
	
	
[SECTION .bss]

StackBase:	resb 2048
StackTop:

[SECTION .text]
	global _start
	global divide_error
	global single_step_exception
	global nmi
	global breakpoint_exception
	global overflow
	global bounds_check
	global inval_opcode
	global copr_not_available
	global double_fault
	global copr_seg_overrun
	global inval_tss
	global segment_not_present
	global stack_exception
	global general_protection
	global page_fault
	global copr_error

	global hwint00
	global hwint01
	global hwint02
	global hwint03
	global hwint04
	global hwint05
	global hwint06
	global hwint07
	global hwint08
	global hwint09
	global hwint10
	global hwint11
	global hwint12
	global hwint13
	global hwint14
	global hwint15


	global restart

	global sys_call

	;; ====================
	
_start:
	mov esp, StackTop
	push StackTop
	call xdisp_u32
	add esp, 4
	
	mov dword [disp_position], (80*20)*2
	
	sgdt [gdt_ptr]
	call init_gdt
	lgdt [gdt_ptr]

	call init_idt
	call init_interrupt
	lidt [idt_ptr]
	
	call init_tss
	xor eax, eax
	mov ax, SELECTOR_TSS
	ltr ax

	call init_keyboard
	;; call init_pit
	
	jmp SELECTOR_KERNEL_CS:_next
_next:
	call kmain


restart:
	mov esp, [ready_process_p]

	lldt [esp + LDT_SELECTOR]
	lea eax, [esp + STACKTOP]
	mov dword [tss + TSS_ESP0], eax
	pop gs
	pop fs
	pop es
	pop ds
	popad
	add esp, 4
	iretd	
	
; 中断和异常 -- 异常
divide_error:
	push	0xFFFFFFFF	; no err code
	push	0		; vector_no	= 0
	jmp	exception
single_step_exception:
	push	0xFFFFFFFF	; no err code
	push	1		; vector_no	= 1
	jmp	exception
nmi:
	push	0xFFFFFFFF	; no err code
	push	2		; vector_no	= 2
	jmp	exception
breakpoint_exception:
	push	0xFFFFFFFF	; no err code
	push	3		; vector_no	= 3
	jmp	exception
overflow:
	push	0xFFFFFFFF	; no err code
	push	4		; vector_no	= 4
	jmp	exception
bounds_check:
	push	0xFFFFFFFF	; no err code
	push	5		; vector_no	= 5
	jmp	exception
inval_opcode:
	push	0xFFFFFFFF	; no err code
	push	6		; vector_no	= 6
	jmp	exception
copr_not_available:
	push	0xFFFFFFFF	; no err code
	push	7		; vector_no	= 7
	jmp	exception
double_fault:
	push	8		; vector_no	= 8
	jmp	exception
copr_seg_overrun:
	push	0xFFFFFFFF	; no err code
	push	9		; vector_no	= 9
	jmp	exception
inval_tss:
	push	10		; vector_no	= A
	jmp	exception
segment_not_present:
	push	11		; vector_no	= B
	jmp	exception
stack_exception:
	push	12		; vector_no	= C
	jmp	exception
general_protection:
	push	13		; vector_no	= D
	jmp	exception
page_fault:
	push	14		; vector_no	= E
	jmp	exception
copr_error:
	push	0xFFFFFFFF	; no err code
	push	16		; vector_no	= 10h
	jmp	exception

exception:
	;; lldt	[process_table + LDT_SELECTOR] ;假设第一个进程的ldt的base是0
	call	exception_handler
	add	esp, 4*2	; 让栈顶指向 EIP，堆栈中从顶向下依次是：EIP、CS、EFLAGS
	hlt




	;; 中断保存现场
save:
	pushad
	push ds
	push es
	push fs
	push gs

	mov edi, edx
	mov dx, ss
	mov es, dx		;
	mov ds, dx		;
	mov fs, dx		;
	mov edx, edi
	mov edi, esp
	;; mov edi, esp
	
	inc dword [reenter]
	cmp dword [reenter] , 0
	jne _reenter
	mov esp, StackTop    	;进 内核栈
	push not_reenter_int
	jmp [edi+48]
_reenter:
	push reenter_int
	
	jmp [edi+48]		;retaddr

not_reenter_int:
	mov esp, [ready_process_p]
	lldt [esp + LDT_SELECTOR]
	lea eax, [esp + STACKTOP]
	mov [tss + TSS_ESP0], eax
reenter_int:
	dec dword [reenter]
	pop gs
	pop fs
	pop es
	pop ds
	popad
	add esp, 4
	iretd

;; 其他一些中断
; ---------------------------------
%macro  hwint_master    1
        call save
	in al, INT_M_CTLMASK	;屏蔽这个中断
	or al, 1<<%1		
	out INT_M_CTLMASK, al
	
	mov al, EOI
	out INT_M_CTL, al
	
	sti
	call [irq_table + 4 * %1]
	cli
	
	in al, INT_M_CTLMASK	;打开这个中断
	and al, ~(1<<%1)	
	out INT_M_CTLMASK, al

	ret
%endmacro
; ---------------------------------

ALIGN   16
hwint00:                ; Interrupt routine for irq 0 (the clock).
	hwint_master   0

	
ALIGN   16
hwint01:                ; Interrupt routine for irq 1 (keyboard)
	hwint_master  1
	
	
ALIGN   16
hwint02:                ; Interrupt routine for irq 2 (cascade!)
        hwint_master    2

ALIGN   16
hwint03:                ; Interrupt routine for irq 3 (second serial)
        hwint_master    3

ALIGN   16
hwint04:                ; Interrupt routine for irq 4 (first serial)
        hwint_master    4

ALIGN   16
hwint05:                ; Interrupt routine for irq 5 (XT winchester)
        hwint_master    5

ALIGN   16
hwint06:                ; Interrupt routine for irq 6 (floppy)
        hwint_master    6

ALIGN   16
hwint07:                ; Interrupt routine for irq 7 (printer)
        hwint_master    7

; ---------------------------------
%macro  hwint_slave     1
	call save
	in al, INT_S_CTLMASK	;屏蔽这个中断
	or al, 1<<%1		
	out INT_S_CTLMASK, al
	
	mov al, EOI
	out INT_M_CTL, al
	nop
	out INT_S_CTL, al
	nop
	
	sti
	call [irq_table + 4 * 8 + 4 * %1]
	cli
	
	in al, INT_S_CTLMASK	;打开这个中断
	and al, ~(1<<%1)	
	out INT_S_CTLMASK, al

	ret
%endmacro
; ---------------------------------

ALIGN   16
hwint08:                ; Interrupt routine for irq 8 (realtime clock).
        hwint_slave     0

ALIGN   16
hwint09:                ; Interrupt routine for irq 9 (irq 2 redirected)
        hwint_slave     1

ALIGN   16
hwint10:                ; Interrupt routine for irq 10
        hwint_slave     2

ALIGN   16
hwint11:                ; Interrupt routine for irq 11
        hwint_slave     3

ALIGN   16
hwint12:                ; Interrupt routine for irq 12
        hwint_slave     4

ALIGN   16
hwint13:                ; Interrupt routine for irq 13 (FPU exception)
        hwint_slave     5

ALIGN   16
hwint14:                ; Interrupt routine for irq 14 (AT winchester)
        hwint_slave     6

ALIGN   16
hwint15:                ; Interrupt routine for irq 15
        hwint_slave     7



	;; ----------------------------------syscall----------------------------
sys_call:
	call save		;'edi' 在save中用作保存进程的esp了，所以调用这个函数的函数不应该使用'edi'
	push dword [ready_process_p]
	
	sti

	push ecx
	push ebx
	call [syscall_table + 4 * eax]
	add esp, 12
	mov dword [edi + 11 * 4], eax	;edx 这里是 原本esp的值 ， 在save里被设定了,
					;; [edx + 11 * 4] 是进程表的stack_frame的eax的位置
	cli
	

	ret
	
	
