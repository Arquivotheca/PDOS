; dosstart.asm - startup code for C programs for DOS
; 
; This program written by Paul Edwards
; Released to the public domain

.model large

extrn ___start:proc

public ___psp
public ___envptr
public ___osver

_DATA   segment word public 'DATA'
banner  db  "PDPCLIB"
___psp   dd  ?
___envptr dd ?
___osver dw ?
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

; add some nops to create a cs-addressable save area, and also create a
; bit of an eyecatcher

nop
nop
nop
nop

; push the psp now, ready for calling start
push ds
mov ax, 0
push ax

; free initially allocated memory

mov bx, 01111h
mov ah, 4ah
int 21h

mov dx,DGROUP
mov ds,dx

mov ah,30h
int 21h
xchg al,ah
mov [___osver],ax

mov word ptr ___psp, 0
mov word ptr [___psp + 2], es
mov word ptr ___envptr, 0
mov dx, es:[02ch]
mov word ptr [___envptr + 2], dx
mov dx, ds
mov es, dx

; we have already pushed the pointer to psp
call far ptr ___start
add sp, 4  ; delete psp from stack

push ax

; how do I get rid of the warning about "instruction can be compacted
; with override"?  The answer is certainly NOT to change the "far" to
; "near".
call far ptr ___exita
add sp, 2
ret
___intstart endp

public ___exita
___exita proc
push bp
mov bp, sp
mov ax, [bp + 6]
mov ah,4ch
int 21h ; terminate
pop bp
ret
___exita endp


public ___main
___main proc
ret
___main endp


_TEXT ends
          
end top
