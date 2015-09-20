	SELECTOR_KERNEL_CS 	equ 	8          
	SELECTOR_TSS		equ 	32
	LDT_SELECTOR            equ     72         ;;sizeof(STACKFRAME)
	STACKTOP		equ 	72
	TSS_ESP0		equ 	4
	EOI 			equ 	020H
	INT_M_CTL 		equ 	020H ;;  I/O port for interrupt controller       <Master>
	INT_M_CTLMASK  		equ   	021H ;;  setting bits in this port disables ints <Master>
	INT_S_CTL 		equ 	0A0H ;;  I/O port for second interrupt controller<Slave> 
	INT_S_CTLMASK 		equ 	0A1H ;;  setting bits in this port disables ints <Slave> 

	SYSCALL_INT_VECTOR	equ     0x80

	SC_get_ticks           	equ 	0     ;;SC   == syscall
	SC_write		equ 	1
	SC_writek		equ 	2
	SC_send			equ 	3
	SC_recv 		equ 	4