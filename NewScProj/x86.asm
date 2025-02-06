.686p

.MODEL FLAT

.code

?strnchr@@YIPADKPBXD@Z proc
	jecxz @retz4
	mov al,[esp + 4]
	xchg edi,edx
	repne scasb
	mov eax,edi
	cmovne eax,ecx
	mov edi,edx
	ret 4
?strnchr@@YIPADKPBXD@Z endp

@retz4 proc
	xor eax,eax
	ret 4
@retz4 endp

@retz8 proc
	xor eax,eax
	ret 8
@retz8 endp

?strnstr@@YIPADKPBXK0@Z proc
	jecxz @retz8
	cmp ecx,[esp + 4]
	jb @retz8
	push edi
	push esi
	push ebx
	push ebp
	mov ebx,[esp + 20]
	mov ebp,[esp + 24]
	mov edi,edx
	mov al,[ebp]
	inc ebp
	dec ebx
	sub ecx,ebx
@@1:
	repne scasb
	jne @@2
	mov esi,ebp
	mov edx,edi
	push ecx
	mov ecx,ebx
	test ecx,ecx
	repe cmpsb
	pop ecx
	je @@2
	mov edi,edx
	jmp @@1
@@2:
	mov eax,edi
	cmovne eax,ecx
	pop ebp
	pop ebx
	pop esi
	pop edi
	ret 8
?strnstr@@YIPADKPBXK0@Z endp

.const

public ?sc_vcxproj_begin@@3QBDB, ?sc_vcxproj_end@@3QBDB

?sc_vcxproj_begin@@3QBDB LABEL BYTE
INCLUDE <vcx.asm>
?sc_vcxproj_end@@3QBDB LABEL BYTE

end