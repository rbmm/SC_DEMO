.686

.MODEL FLAT

.code

; _ERW_ = 1

; void __stdcall ep()
extern ?ep@@YGXXZ : PROC

; void __stdcall epASM()
?epASM@@YGXXZ proc

IFNDEF _ERW_
	call protect
ENDIF

	jmp ?ep@@YGXXZ
?epASM@@YGXXZ endp

include <../scentry/nobase.x86.inc>

include <imp.x86.asm>

end