__chkstk proc
;_alloca_probe proc
    sub         rsp,10h
    mov         qword ptr [rsp],r10
    mov         qword ptr [rsp+8],r11
    xor         r11,r11
    lea         r10,[rsp+18h]
    sub         r10,rax
    cmovb       r10,r11
    mov         r11,qword ptr gs:[10h]
    cmp         r10,r11
    jae         @@1
    and         r10w,0F000h
 @@0:   
    lea         r11,[r11-1000h]
    test        byte ptr [r11],r11b
    cmp         r10,r11
    jb          @@0
 @@1:   
    mov         r10,qword ptr [rsp]
    mov         r11,qword ptr [rsp+8]
    add         rsp,10h
    ret
;_alloca_probe endp
__chkstk endp

createAstring macro name, string
name proc
	lea rax,@@1
	ret
@@1:
	DB string,0
name endp
endm

WSTRING macro text
	FORC arg, text
	DW '&arg'
	ENDM
	DW 0
endm

createWstring macro name, string
name proc
	lea rax,@@1
	ret
	ALIGN 2
@@1:
	WSTRING string
name endp
endm

HMOD macro name, str
name proc private
	lea r11,@@1
	ret
	ALIGN 8
@@1:
	DQ 0
	WSTRING str
name endp
endm

createFunc macro hmod, name, hash
name proc
	call hmod
	lea r10,@@1
	jmp common_imp_call
	ALIGN 8
@@1: 
	DQ 0
	DD hash
name endp
endm

; void *__cdecl get_hmod(const wchar_t *)
extern ?get_hmod@@YAPEAXPEB_W@Z : PROC

; void *__cdecl GetFuncAddressEx(struct _IMAGE_DOS_HEADER *,unsigned long)
extern ?GetFuncAddressEx@@YAPEAXPEAU_IMAGE_DOS_HEADER@@K@Z : PROC

; r10 -> func, r11 -> hmod
common_imp_call proc private
  mov rax,[r10] ; rax = func
  test rax,rax ; we already have func address ?
  jnz @@1
  push r9
  push r8
  push rdx
  push rcx
  push r10
  push r11
  sub rsp,28h
  mov rax,[r11] ; rax = hmod
  test rax,rax ; we already load dll ?
  jnz @@2
  lea rcx,[r11 + 8]
  call ?get_hmod@@YAPEAXPEB_W@Z
  mov r11,[rsp + 28h]
  mov r10,[rsp + 30h]
  mov [r11],rax ; hmod = get_hmod, save dll address
@@2:
  mov edx,[r10 + 8]
  mov rcx,rax
  call ?GetFuncAddressEx@@YAPEAXPEAU_IMAGE_DOS_HEADER@@K@Z
  add rsp,28h
  pop r11
  pop r10
  pop rcx
  pop rdx
  pop r8
  pop r9
  mov [r10],rax ; func = GetFuncAddressEx, save func address
@@1:
  jmp rax
common_imp_call endp