; nearw.asm - assembler support functions for Watcom
;
; This program written by Paul Edwards
; Released to the public domain

% .model memodel

;extrn ___divide:proc
;extrn ___modulo:proc

_DATA   segment word public 'DATA'
_DATA   ends
_BSS    segment word public 'BSS'
_BSS    ends

DGROUP  group   _DATA,_BSS
        assume cs:_TEXT,ds:DGROUP

_TEXT segment word public 'CODE'

ifdef WATCOM
; divide dx:ax by cx:bx, result in dx:ax
public __U4D
__U4D proc
push cx
push bx
push dx
push ax
push cx
push bx
push dx
push ax
call f_lumod@
mov cx, dx
mov bx, ax
call f_ludiv@
ret
__U4D endp
endif


public f_ludiv@
f_ludiv@ proc
push bp
mov bp,sp
push bx

cmp word ptr [bp + 10], 0
jne ludiv_full

mov ax, [bp + 6]
mov dx, 0
div word ptr [bp + 8]
mov bx, ax
mov ax, [bp + 4]
div word ptr [bp + 8]

mov dx, bx
jmp short ludiv_fin

ludiv_full:
push word ptr [bp + 10]
push word ptr [bp + 8]
push word ptr [bp + 6]
push word ptr [bp + 4]
;call far ptr ___divide
add sp, 8

ludiv_fin:

pop bx
pop bp
ret 8
f_ludiv@ endp


; procedure needs to fix up stack
public f_lumod@
f_lumod@ proc
push bp
mov bp,sp

cmp word ptr [bp + 10], 0
jne lumod_full

mov ax, [bp + 6]
mov dx, 0
div word ptr [bp + 8]
mov ax, [bp + 4]
div word ptr [bp + 8]
mov ax,dx
mov dx, 0
jmp short lumod_fin

lumod_full:
push word ptr [bp + 10]
push word ptr [bp + 8]
push word ptr [bp + 6]
push word ptr [bp + 4]
;call far ptr ___modulo
add sp, 8

lumod_fin:

pop bp
ret 8
f_lumod@ endp


; multiply cx:bx by dx:ax, result in dx:ax

public __I4M
__I4M:
public __U4M
__U4M:
public f_lxmul@
f_lxmul@ proc
push bp
mov bp,sp
push cx

push ax
mul cx
mov cx, ax
pop ax
mul bx
add dx, cx

pop cx
pop bp
ret
f_lxmul@ endp

_TEXT ends

end
