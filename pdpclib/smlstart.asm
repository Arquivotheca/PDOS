; smlstart.asm - startup code for C programs written
; in the Smaller C memory model to be run under MSDOS
;
; This program written by Paul Edwards
; Released to the public domain

bits 16

section .text
	global	___intstart
___intstart:
	push	ebp
	movzx	ebp, sp
	mov	eax, 5
L1:
	db	0x66
	leave
	retf
L3:


        global  ___exita
___exita:
        retf


        global  ___creat
___creat:
        retf


        global  ___open
___open:
        retf


        global  ___close
___close:
        retf


        global  ___read
___read:
        retf


        global  ___write
___write:
        retf


        global  ___remove
___remove:
        retf


        global  ___rename
___rename:
        retf


        global  ___seek
___seek:
        retf


        global  ___lesf2
___lesf2:
        retf


        global  ___mulsf3
___mulsf3:
        retf


        global  ___floatsisf
___floatsisf:
        retf


        global  ___addsf3
___addsf3:
        retf


        global  ___negsf2
___negsf2:
        retf


        global  ___gesf2
___gesf2:
        retf


        global  ___divsf3
___divsf3:
        retf


        global  ___fixsfsi
___fixsfsi:
        retf


        global  ___subsf3
___subsf3:
        retf


extern  ___start

global ___psp
global ___envptr
global ___osver

section .data
         align 4
banner:
         db  "PDPCLIB"
___psp:
;         dd  ?
___envptr:
;         dd ?
___osver:
;         dw ?

;top:
;
;___intstart proc

; add some nops to create a cs-addressable save area, and also create a
; bit of an eyecatcher

;nop
;nop
;nop
;nop

; push the psp now, ready for calling start
;push ds
;mov ax, 0
;push ax

; determine how much memory is needed. The stack pointer points
; to the top. Work out what segment that is, then subtract the
; starting segment (the PSP), and you have your answer.

;mov ax, sp
;mov cl, 4
;shr ax, cl ; get sp into pages
;mov bx, ss
;add ax, bx
;add ax, 2 ; safety margin because we've done some pushes etc
;mov bx, es
;sub ax, bx ; subtract the psp segment

; free initially allocated memory

;mov bx, ax
;mov ah, 4ah
;int 21h

;mov dx,DGROUP
;mov ds,dx

;mov ah,30h
;int 21h
;xchg al,ah
;mov [___osver],ax

;mov word ptr ___psp, 0
;mov word ptr [___psp + 2], es
;mov word ptr ___envptr, 0
;mov dx, es:[02ch]
;mov word ptr [___envptr + 2], dx
;mov dx, ds
;mov es, dx

; we have already pushed the pointer to psp
;call far ptr ___start
;add sp, 4  ; delete psp from stack

;push ax

; how do I get rid of the warning about "instruction can be compacted
; with override"?  The answer is certainly NOT to change the "far" to
; "near".
;call far ptr ___exita
;add sp, 2
;ret
;___intstart endp

;public ___exita
;___exita proc
;push bp
;mov bp, sp
;mov ax, [bp + 6]
;mov ah,4ch
;int 21h ; terminate
;pop bp
;ret
;___exita endp


;public ___main
;___main proc
;ret
;___main endp


;_TEXT ends

;end top
