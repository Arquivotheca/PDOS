; needpdos.asm - stub for Win32 executables
;
; This program written by Paul Edwards
; Released to the public domain
;
; Assemble with tasm -l needpdos.asm
; Link with tlink needpdos

.model tiny

_DATA   segment word public 'DATA'
msg  db  "Install HX or upgrade to PDOS/386 or Wine etc"
msg2 db  0DH
msg3 db  0AH
msg4 db "$"
_DATA   ends
_BSS    segment word public 'BSS'
_BSS    ends
_STACK  segment word stack 'STACK'
        db 1000h dup(?)
_STACK  ends

DGROUP  group   _DATA,_BSS
        assume cs:_TEXT,ds:DGROUP

_TEXT segment word public 'CODE'

top:

___intstart proc

; I am suspicious about how this DGROUP gets set. Although it seems
; to work, it appears as x'0000' in the executable, so it must be
; dependent on being relocated. But we don't want relocations to
; be done, as we are only changing the code, not the header. So
; switching DS (pointing to PSP) to CS is the right thing to do.
;mov dx,DGROUP
;mov ds,dx

push cs
pop ds

mov ah,09h
mov dx,offset msg
int 21h

mov al,1 ; set an error return code
mov ah,4ch
int 21h ; terminate

; No need for a return. We're screwed anyway if the above
; doesn't work. There's nothing on the stack to return to.
;ret

___intstart endp


_TEXT ends

end top
