EXTERN _S_Rva:QWORD
EXTERN _S_hnt:QWORD

.code

common_syscall proc private
	rdrand r10
	movzx r10,r10b
	mov r11,_S_Rva
	mov r11d,[r11 + 4 * r10]
	movzx eax,ax
	mov r10,rcx
	test r11d,r11d
	jz @@0
	add r11,_S_hnt
	jmp r11
@@0:
	syscall
	ret
common_syscall endp

public ZwFuncs

ZwFuncs LABEL BYTE
	N = 0
	REPEAT 512
		mov ax,N
		jmp near ptr common_syscall
		N = N + 1
    ENDM

end
