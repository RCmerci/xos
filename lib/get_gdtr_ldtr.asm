[SECTION .text]
	global	 get_ldtr


get_ldtr:
	push ebp
	mov ebp, esp

	xor eax, eax
	sldt ax
	
	mov esp, ebp
	pop ebp
	ret

;; get_gdtr:
;; 	push ebp
;; 	mov ebp, esp

;; 	xor eax, eax
;; 	sgdt ax
	
;; 	mov esp, ebp
;; 	pop ebp
;; 	ret
	
	
