; needpdos.asm - stub for Win32 executables
;
; This program written by Paul Edwards
; Released to the public domain

.model tiny

_DATA   segment word public 'DATA'
msg  db  "Please install HX or upgrade to PDOS/386 or Wine etc etc$"
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

mov dx,DGROUP
mov ds,dx

mov ah,09h
mov dx,offset msg
int 21h

mov al,0
mov ah,4ch
int 21h ; terminate

ret

___intstart endp


_TEXT ends

end top
