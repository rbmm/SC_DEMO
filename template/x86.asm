.686

.MODEL FLAT

.code

; void __stdcall ep()
extern ?ep@@YGXXZ : PROC

; void __stdcall epASM()
?epASM@@YGXXZ proc
  jmp ?ep@@YGXXZ
?epASM@@YGXXZ endp

include <../scentry/nobase.x86.inc>

include <imp.x86.asm>

end