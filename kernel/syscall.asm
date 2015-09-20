%include "xconst.inc"
	;; 全局变量
	extern ticks
	;; 导入函数
	

	;; 导出
	;; global get_ticks	
	;; global write
	;; global writek
	global send
	global recv_
	;; 系统调用
	;; 
[section .text]
;; get_ticks:
;; 	mov eax, SC_get_ticks
;; 	int SYSCALL_INT_VECTOR
;; 	ret

;; write:
;; 	mov eax, SC_write
;; 	mov ebx, [esp+4]
;; 	mov ecx, [esp+8]
;; 	int SYSCALL_INT_VECTOR
;; 	ret

;; writek:
;; 	mov eax, SC_writek
;; 	mov ebx, [esp+4]
;; 	mov ecx, [esp+8]
;; 	int SYSCALL_INT_VECTOR
;; 	ret
	
	

send:				
	mov eax, SC_send
	mov ebx, [esp+4]
	mov ecx, [esp+8]
	int SYSCALL_INT_VECTOR
	ret

recv_:				
	mov eax, SC_recv
	mov ebx, [esp+4]
	mov ecx, [esp+8]
	int SYSCALL_INT_VECTOR
	ret
	
