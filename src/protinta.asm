; This program written by Paul Edwards and Matthew Parker
; released to the public domain

; protinta.asm - assembler portion of protected mode interface 

; rawprota - called by rawprot.  Assembler portion of real mode
; to protected mode (and back to real) routine.

; ptor and rtop - internal functions for one-way mode transition.

; The process for switching to real mode from protected mode is:

; 1. Set DS, ES etc to have a limit of 64k.
; 2. Set CS to have a limit of 64k via a far jump
; 3. Switch off the protected mode bit
; 4. Do a far jump to wherever in real memory you want

; For more information see 386INTEL.ZIP on the internet.

% .model memodel,c
  .386p

extrn realsub:proc
extrn gdtinfo:fword
extrn idtinfo:fword
extrn ridtinfo:fword

public rawprota
public runreal
public newstack
public rtop_stage2
public protget32

_DATA   segment word public 'DATA'
; used for initial protected mode jump
joffs   dd ?

loffs   dd ?

; for ptor transitions
poffs   dw offset ptor_stage2
pseg    dw 018h

qoffs   dw offset ptor_stage3
qseg    dw ?

; for rtop transitions
roffs   dd 0
rseg    dw 8

oldstack label dword
oldss   dw ?
oldsp   dw ?

newstack dd ?

oldds   dw ?
_DATA   ends

_BSS    segment word public 'BSS'
_BSS    ends

assume cs:_TEXT
_TEXT segment word use16 public 'CODE'

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; unsigned long rawprota(...);
; ***********************************************************
; switch to protected mode and call passed routine, passing it
; the passed pointer.  Also needs the new stack and also the
; code corrections for use in protected mode.
; This function calls rtop to make the switch, then does a call
; to a protected mode function, then calls ptor to switch back.
; It also initializes some values needed by rtop and ptor.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

rawprota proc corsubr:dword, \
              codecor:dword, \
              newsp:dword, \
              parmlist:dword
        push esi
        push edi
        push ecx
        push ebx
        pushf

; go and set values for return to real mode
        mov ax,cs
        mov qseg,ax
        
        mov ax,ds
        mov oldds, ax

; get correction required for code offsets into eax
        mov eax, codecor
        
; now adjust the various places that require the code offsets        
        mov joffs, offset rawprota_stage2
        add joffs, eax        

        mov loffs, offset runreal_stage3
        add loffs, eax

        mov roffs, offset rtop_stage2
        add roffs, eax

        mov edx, offset runreal
        add edx, eax
; end up storing one offset in edx to be passed up        

; save current ss:sp into variables for when we do real mode
; interrupts
        mov ax, ss
        mov oldss, ax
        mov ax, sp
        mov oldsp, ax

        mov eax, newsp
        mov newstack, eax
        
        mov esi, parmlist
        mov ecx, corsubr
; jump to rawprota_stage2        
        mov ebx, joffs
        jmp rtop
          
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
rawprota_stage3 label near
; protected mode subroutine has terminated
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; put edi (where we saved eax, the return code) into dx:ax
        mov ax, di
        mov edx, edi
        shr edx, 16
        popf
        pop ebx
        pop ecx        
        pop edi
        pop esi
        ret
rawprota endp


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
runreal_stage2:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        
        push esi
        push edi
        call realsub
        add sp, 8
        
; call protected mode runreal_stage3
        mov ebx, loffs
        jmp rtop
        
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; end of runreal_stage2
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; rtop - go from real mode to protected mode
; ***********************************************************
; It expects newstack to be already set
; And gdtinfo and idtinfo to be set
; And the routine to jump to to be in ebx
; eax will be clobbered in the process
; interrupts will be disabled by this routine
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

rtop:
        cli
        mov esp, newstack
        lgdt fword ptr gdtinfo
        lidt fword ptr idtinfo
        mov eax, cr0
        or eax, 1
        mov cr0, eax
        
; Now because we can't easily do a jmp 8:rtop_stage2, we instead do an
; indirect far jump
        jmp fword ptr roffs


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ptor_stage2:
; switch to real mode and then do a far jump
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        mov cr0, eax
;        jmp ptor_stage3
; Note, I would have liked to do a jmp far ptr [qoffs] here,
; but unfortunately I had to bodge around doing a far return
; instead.  And I need to set the stack before doing that too!
        mov newstack, esp
        mov ax, cx
        shr ecx, 16
        mov ss, ax
        mov sp, cx
        push qseg
        push qoffs
        retf

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ptor_stage3:
; load real mode interrupts and jump to routine specified
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; This is the restoration of oldds
        mov ds, dx

        lidt fword ptr ridtinfo
        jmp ebx
        
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; unsigned long protget32(void);
; ***********************************************************
; get the far address of the _TEXT32 module
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
protget32 proc
; if it's the tiny memory model, return 0 and let the C code
; handle it.
if @Model eq 1
        mov dx, 0
        mov ax, 0
else
        mov dx, _TEXT32
        mov ax, offset _TEXT32
endif
        ret
protget32 endp
_TEXT ends


_TEXT32 segment dword use32 public 'CODE'
assume cs:_TEXT32

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; rtop_stage2 - go from real mode to protected mode part2
; This is the first time we are actually running in protected mode
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

rtop_stage2:
        mov ax, 0010h 
        mov ss, ax
        mov gs, ax
        mov fs, ax
        mov es, ax
        mov ds, ax
        jmp ebx
        

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; ptor - go from protected mode to real mode
; ***********************************************************
; you must have set ebx to the address you want to jump to
; eax, edx and ecx are clobbered by this routine
; also oldstack must have been set up already
; also oldds must have been set up already
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ptor:
        cli
        mov ax, 020h
        mov ss, ax
        mov gs, ax
        mov fs, ax
        mov es, ax
        mov ds, ax
        mov eax, cr0
        and eax, 0fffffffeh
        
        mov ecx, oldstack
        mov dx, oldds

;        jmp ptor_stage2
; Note - I really want to do a jmp far ptr [poffs] here but it
; doesn't generate the right code, so instead we dummy it up.        
        db 66h
        jmp fword ptr poffs


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; rawprota_stage2 - call protected mode function
; ecx has the address of the protected mode entry point in it.
; esi has the parameter for it
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

rawprota_stage2:
        push esi  ; protected mode parameter
        call ecx  ; call protected mode routine (address in ebx)
        add esp, 4     ; skip parameter

        mov edi, eax   ; save return code
; Return to real mode
        mov ebx, offset rawprota_stage3
        jmp ptor
        
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; unsigned long runreal(unsigned long func, unsigned long parm);
; ***********************************************************
runreal proc
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        push ebp
        mov ebp, esp
        push esi
        push edi
        push edx
        push ecx
        push ebx

        mov edi, [ebp + 8]
        mov esi, [ebp + 12]
        
; Return to real mode
        mov ebx, offset runreal_stage2
        jmp ptor
        
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
runreal_stage3 label near
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; convert return long return code into 32 bit int
        shl edx, 16  ; preserve the contents of dx by shifting
        mov dx, ax   ; combine with contents of ax
        mov eax, edx ; move the new value into eax
        
        pop ebx
        pop ecx        
        pop edx
        pop edi
        pop esi
        pop ebp
        retn    ; always near return since called from 32-bit
runreal  endp

_TEXT32 ends

end
