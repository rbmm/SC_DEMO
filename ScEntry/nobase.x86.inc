createAstring macro name, string
name proc
  call @@1
  DB string,0
@@1:
  pop eax
  ret
name endp
endm

WSTRING macro text
  FORC arg, text
  DW '&arg'
  ENDM
  DW 0
endm

createWstring macro name, string
  ALIGN 2
  nop
name proc
  call @@1
  WSTRING string
@@1:
  pop eax
  ret
name endp
endm

HMOD macro name, str
  ALIGN 4
  nop
  nop
  nop
name proc private
  call @@1
  DD 0
  WSTRING str
@@1:
  pop ecx

  ret
name endp
endm

createFunc macro hmod, name, hash
  ALIGN 4
  nop
  nop
name proc
  call hmod
  call @@1
  DD 0
  DD hash
@@1: 
  pop edx
  jmp common_imp_call
name endp
endm

; void *__fastcall get_hmod(const wchar_t *)
extern ?get_hmod@@YIPAXPB_W@Z : PROC

; void *__fastcall GetFuncAddressEx(struct _IMAGE_DOS_HEADER *,unsigned long)
extern ?GetFuncAddressEx@@YIPAXPAU_IMAGE_DOS_HEADER@@K@Z : PROC

; ecx -> hmod, edx -> func
common_imp_call proc private
  mov eax,[edx] ; eax = func
  test eax,eax ; we already have func address ?
  jnz @@1
  push edx
  mov eax,[ecx] ; eax = hmod
  test eax,eax ; we already load dll ?
  jnz @@2
  push ecx
  lea ecx,[ecx + 4]
  call ?get_hmod@@YIPAXPB_W@Z
  pop ecx
  mov [ecx],eax ; hmod = get_hmod, save dll address
  mov edx,[esp]
@@2:
  mov edx,[edx + 4]
  mov ecx,eax
  call ?GetFuncAddressEx@@YIPAXPAU_IMAGE_DOS_HEADER@@K@Z
  pop edx
  mov [edx],eax ; func = GetFuncAddressEx, save func address
@@1:
  jmp eax
common_imp_call endp

; void *__fastcall __Address(const void * pv)
; do relocation in runtime
; pv : address valid with preffered base
; return : actual address inside ShellCode

?__Address@@YIPAXPBX@Z proc
	call @@0
@@0:
	pop eax
	lea edx,?__Address@@YIPAXPBX@Z
	sub ecx,edx
	lea eax,[ecx + eax - 5]
	ret
?__Address@@YIPAXPBX@Z endp

__chkstk proc
__alloca_probe proc
   push        ecx
   lea         ecx,[esp+4]
   sub         ecx,eax
   sbb         eax,eax
   not         eax
   and         ecx,eax
   mov         eax,esp
   and         eax,0FFFFF000h
@@1:
   cmp         ecx,eax
   jb          @@2
   mov         eax,ecx
   pop         ecx
   xchg        eax,esp
   mov         eax,dword ptr [eax]
   mov         dword ptr [esp],eax
   ret
@@2:
   sub         eax,1000h
   test        dword ptr [eax],eax
   jmp         @@1
__alloca_probe endp
__chkstk endp






