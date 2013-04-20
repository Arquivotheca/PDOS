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

; HD:
;
; 000000  EB479050 444F5320 782E78
;                                 00 02        bytes per sector
;                                      04      sectors per cluster
;                                        0100  # reserved
;                                              .G.PDOS x.x.....
; 000010  02                                   # fats
;           0002                               # directory entries (512)
;               01 EC                          total logical sectors
;                    F8                        media byte
;                      3B00                    sectors per FAT
; 
; 3b=59*2 + 32 (directory sectors) + 1 (reserved) = 151
; 
;                           3F001000 3F000000  ......;.?...?...
; 000020  00000000 800029EC 1C591E4E 4F204E41  ......)..Y.NO NA
; 000030  4D452020 20204641 54313620 202031C0  ME    FAT16   1.
; 000040  FA8ED0BC 007CFB0E 1FB80000 8ED0BC00  .....|..........
; 000050  7C0E1FE8 5D00B870 0050B800 0050CB06  |...]..p.P...P..
; 000060  53505152 1EB80000 8EC0BB78 00268B47  SPQR.......x.&.G
; 000070  028ED826 8B078BD0 B800008E C0BB3E7C  ...&..........>|
; 000080  B9000083 F90B7D0E 538BDA8A 075B2688  ......}.S....[&.
; 000090  07424341 EBEDB800 008EC0BB 780026C7  .BCA........x.&.
; 
; floppy:
; 
; 000000  EB3C904C 494E5558 342E31
;                                 00 02        bytes per sector
;                                      01      sectors per cluster
;                                        0100  # reserved
;                                              .<.LINUX4.1.....
; 000010  02                                   # fats
;           E000                               # directory entries (224)
;               40 0B                          total logical sectors
;                    F0                        media byte
;                      0900                    sectors per FAT
; 
; 9*2 + 224*32/512 + 1 = 33
;                           12000200 00000000  ...@............
; 000020  00000000 000029E3 7FFA4420 20202020  ......)...D
; 000030  20202020 20204641 54313220 2020FAFC        FAT12   ..
; 000040  31C08ED8 BD007CB8 E01F8EC0 89EE89EF  1.....|.........
; 000050  B90001F3 A5EA5E7C E01F0000 60008ED8  ......^|....`...
; 000060  8ED08D66 A0FB807E 24FF7503 885624C7  ...f...~$.u..V$.
; 000070  46C01000 C746C201 00E8E900 46726565  F....F......Free
; 000080  444F5300 8B761C8B 7E1E0376 0E83D700  DOS..v..~..v....
; 000090  8976D289 7ED48A46 1098F766 1601C611  .v..~..F...f....


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
mov cl, 26  ; +++ needs to be 010h for floppy
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
; dl should not have changed since boot time
; mov dl, 080h ; drive

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
mov dh, 03h ; head   ; +++ should be 01h for floppy
; dl should not have changed since boot time
; mov dl, 080h ; drive

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

