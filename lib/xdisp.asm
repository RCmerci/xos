	extern 	disp_position	;显示的位置
[SECTION .text]
	global 	xdisp_al
	global 	xdisp_color_str
	global  xdisp_u32
	global  xdisp_word_at
	;; -------------------------------------------------------------
xdisp_al:
	;; 黑底白字
	;; 要显示的数字在 al
	push ebp
	mov ebp, esp
	push ebx
	push ecx
	;;
	mov ecx, 2
	mov edi, [disp_position]
	add edi, 2		;先显示一位（XXH）
_start_xdisp_al:	
	xor ah, ah
	mov bl, 10h
	div bl
	cmp ah, 0AH
	jl _less_xdisp_al
_greater_xdisp_al:
	mov bl, ah
	sub bl, 0Ah
	add bl, 'A'
	mov bh, 0Fh
	mov [gs:edi], bx
	jmp _end_xdisp_al

_less_xdisp_al:
	mov bl, ah
	add bl, '0'
	mov bh, 0Fh
	mov [gs:edi], bx
	
_end_xdisp_al:
	sub edi, 2
	loop _start_xdisp_al
	add edi, 6
	mov [disp_position] ,edi
	pop ecx
	pop ebx
	mov esp, ebp
	pop ebp
	ret
	;; ------------------------------------------------------------
xdisp_color_str:
	;; func(str, color)
	push ebp
	mov ebp, esp
	push esi
	mov esi, [ebp + 8]
	mov ebx, [ebp + 12]
	mov edi, [disp_position]
_loop_xdisp_color_str:	
	mov al, [esi]
	inc esi
	cmp al, 0
	jz _end_xdisp_color_str
	cmp al, 0AH		;\n?
	jnz _goon_xdisp_color_str ;jmp if not \n
	push eax
	mov eax, edi
	mov bl, 160
	div bl
	and eax, 0FFH
	inc eax
	mov bl, 160
	mul bl
	mov edi, eax
	pop eax
	jmp _loop_xdisp_color_str
_goon_xdisp_color_str:		
	mov ah, bl
	mov [gs:edi], ax
	add edi, 2
	jmp _loop_xdisp_color_str
_end_xdisp_color_str:	
	;;
	mov [disp_position], edi
	mov esp, ebp
	pop ebp
	ret


	;; -------------------------------------------------------
xdisp_u32:
	;; xdisp_u32(u32)
	push ebp
	mov ebp, esp
	push dword [disp_position]
	;; 
	mov eax, [ebp+8]
	add dword [disp_position], 6*2
	;;
	push eax
	call xdisp_al
	pop eax
	;; 
	sub dword [disp_position], 4*2
	shr eax, 8
	push eax
	call xdisp_al
	pop eax
	;; 
	sub dword [disp_position], 4*2
	shr eax, 8
	push eax
	call xdisp_al
	pop eax
	;; 
	sub dword [disp_position], 4*2
	shr eax, 8
	call xdisp_al
	;; 
	pop dword [disp_position]
	mov eax, [disp_position]
	add eax, 8*2
	mov [disp_position], eax
	mov esp, ebp
	pop ebp
	ret
	
xdisp_word_at:
	push ebp
	mov ebp, esp
	push edi
	
	mov ax, [ebp + 8]
	mov edi, [ebp + 12]

	mov [gs:edi], ax

	pop edi
	mov esp, ebp
	pop ebp
	ret
