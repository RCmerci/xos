[SECTION .text]
	global xmemcpy

	
xmemcpy:
	;; memcpy(dest, src, len)
	push ebp
	mov ebp, esp
	
	push edi
	push esi
	push ecx

	
	mov edi, [ebp+8]
	mov esi, [ebp+12]
	mov ecx, [ebp+16]

	cmp ecx, 0
	jng  _end		;ecx <= 0
_loop:
	mov al, [esi]
	mov [edi], al
	inc esi
	inc edi
	
	loop _loop

_end:
	mov eax, [ebp+8]

	;;
	pop ecx
	pop esi
	pop edi
	mov esp, ebp
	pop ebp
	
	ret
