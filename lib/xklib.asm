%include "xconst.inc"

	[SECTION .text]
	global xout_byte
	global xin_byte
	global disable_int
	global enable_int
	global disable_irq
	global enable_irq
	global port_read
	global port_write

xout_byte:
	;; out_byte(dest, data)
	mov al, [esp+8] 	
	mov dx, [esp+4]

	out dx, al
	nop			; 要加点延迟
	nop
	ret


xin_byte:
	;; al = in_byte(dest)
	mov dx,[esp+4]
	in al, dx
	nop
	nop
	ret

	;; ----------------------------------------------------
disable_int:
	cli
	ret
enable_int:
	sti
	ret

	;; ----------------------------------------------------
disable_irq:			;e.g. disable_irq(KEYBOARD_IRQ)
	push ebp
	mov ebp, esp
	
	mov ecx, [ebp+8]

	mov ah, 1
	rol ah, cl

	cmp ecx, 8

	jae  _less_disable_irq
	
	in al,  INT_M_CTLMASK
	or al, ah
	out INT_M_CTLMASK, al
	jmp _end_disable_irq
_less_disable_irq:
	in al, INT_S_CTLMASK
	or al, ah
	out INT_S_CTLMASK, al
_end_disable_irq:
	mov esp, ebp
	pop ebp
	ret


enable_irq:
	push ebp
	mov ebp, esp

	mov ecx, [ebp+8]

	mov ah, ~1
	rol ah, cl

	cmp ecx, 8

	jae  _less_enable_irq
	
	in al,  INT_M_CTLMASK
	and al, ah
	out INT_M_CTLMASK, al
	jmp _end_disable_irq
_less_enable_irq:
	in al, INT_S_CTLMASK
	and al, ah
	out INT_S_CTLMASK, al
_end_enable_irq:
	mov esp, ebp
	pop ebp
	ret


	
;                  void port_read(u16 port, void* buf, int n);
port_read:
	mov	edx, [esp + 4]		; port
	mov	edi, [esp + 4 + 4]	; buf
	mov	ecx, [esp + 4 + 4 + 4]	; n
	shr	ecx, 1
	cld
	rep	insw
	ret

;                  void port_write(u16 port, void* buf, int n);
port_write:
	mov	edx, [esp + 4]		; port
	mov	esi, [esp + 4 + 4]	; buf
	mov	ecx, [esp + 4 + 4 + 4]	; n
	shr	ecx, 1
	cld
	rep	outsw
	ret
