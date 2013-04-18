; pbootsec.asm - pdos boot sector
; 
; This program written by Paul Edwards
; Released to the public domain

; This code will be loaded to location 07C00h by the BIOS
; We set the stack at 07C00h and make it grow down

; Registers at entry are:
; CS:IP = 07c00H, probably 0000:7c00
; DL = drive number
; nothing else guaranteed

; We should store the drive number in BPB[25], since
; pload is expecting it there.  pload also expects
; that the CS:IP will have an IP of 0 when it is called.
; It also expects some sort of stack to be set up.

% .model memodel

_DATA   segment word public 'DATA'
_DATA   ends
_BSS    segment word public 'BSS'
_BSS    ends

_TEXT segment word public 'CODE'

org 0100h
top:

public start
start proc

; jump around buffer
jmp bypass
nop

sysname db 'PDOS x.x'
bpb db 51 dup(?)

; new disk parameter table
dpt db 11 dup(?)


bypass:
mov ax, 0
mov ss, ax
mov sp, 07c00h

push cs
pop ds

; don't bother setting new dpt, it doesn't appear to be required
;call newdpt
;call reset
call loadsecs

;jmp 0070h:0000h
mov ax, 0070h
push ax
mov ax, 0
push ax
retf

start endp


; newdpt - set up new dpt
newdpt proc

; old dpt is located at interrupt 1E = 078h
push es
push bx
push ax
push cx
push dx
push ds

mov ax, 0
mov es, ax
mov bx, 078h
mov ax, es:[bx + 2]
mov ds, ax
mov ax, es:[bx]
mov dx, ax

mov ax, 0
mov es, ax
mov bx, 07C3Eh
mov cx, 0
newdpt_loop:
cmp cx, 11
jge newdpt_end
push bx
mov bx, dx
mov al, [bx]
pop bx
mov es:[bx], al
inc dx
inc bx
inc cx
jmp newdpt_loop
newdpt_end:

mov ax, 0
mov es, ax
mov bx, 078h
mov word ptr es:[bx + 2], 0
mov word ptr es:[bx], 07C3Eh
mov bx, 07C3Eh

; set offset 4 to 18 (should be number of sectors per track)
;mov byte ptr es:[bx + 4], 18

pop ds
pop dx
pop cx
pop ax
pop bx
pop es
ret

newdpt endp


; loadsecs - load sectors
loadsecs proc

; read 3 sectors
mov bx, 0700h
mov cl, 26
call saferead
add bx, 0200h
add cl, 1
call saferead
add bx, 0200h
add cl, 1
call saferead
ret

loadsecs endp


; saferead
saferead proc

mov ax, 0
jmp first
retry:
call reset
inc ax
cmp ax, 05h
jge giveup
first:
call secread
jnz retry
giveup:
ret

saferead endp


; reset
reset proc

mov ah, 00h ; function
mov dl, 080h ; drive

int 013h

ret

reset endp


; secread - bx = offset, cl = sector
secread proc

push bx
push cx
push ax

mov ax, 0h
mov es, ax  ; buffer segment
mov ah, 02h ; code
mov al, 01h ; 1 sector
mov ch, 00h ; track 0 
mov dh, 03h ; head
mov dl, 080h ; drive

int 013h

pop ax
pop cx
pop bx
ret

secread endp


org 02feh
lastword dw 0aa55h

_TEXT ends
          
end top

