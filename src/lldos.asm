; Released to the public domain by Matthew Parker on 4 December 1995
; For different models just change the .model directive

ifdef WATCOM
.model memodel,c
else
% .model memodel, c
endif

ifdef WATCOM
getfar   equ _getfar
putfar   equ _putfar
rportb  equ _rportb
wportb equ _wportb
enable   equ _enable
disable  equ _disable
callfar equ _callfar
callwithpsp equ _callwithpsp
callwithbypss equ _callwithbypass
a20e equ _a20e
reboot equ _reboot
putabs equ _putabs
getabs equ _getabs
endif

ifdef NEED_HANDLER
extrn int21:proc
public instint, handler,
endif

public getfar, putfar, rportb, wportb, enable, disable
public callfar, callwithpsp, callwithbypass, a20e
public reboot, putabs, getabs

        .code
getfar proc, address: dword
        push ds
        lds bx,address
        mov ah,0
        mov al,[bx]
        pop ds
        ret
getfar endp

putfar proc, address2: dword, character: word
        push ds
        lds bx,address2           ;We recieve a word yet
        mov al,byte ptr character ;we only put a char.
        mov [bx],al
        pop ds
        ret
putfar endp

rportb proc, port: word
        mov dx,port
        in al,dx
        ret
rportb endp

wportb proc, port2: word, outbyte: byte
        mov dx,port2
        mov al,outbyte
        out dx,al
        ret
wportb endp

enable  proc 
        sti
        ret
enable  endp

disable proc 
        cli
        ret
disable endp

callfar proc, address: dword
        push bx
        push ds
        push ax
        
        lds bx, address
        lea ax, callfar_ret
        push cs
        push ax
        push ds
        push bx
        
        retf    ; call desired routine
callfar_ret:
        pop ax
        pop ds
        pop bx
        ret
callfar endp

callwithpsp proc, address: dword, psp: dword, ss_new: word, sp_new: word
        push bx
        push ds
        push cx
        push es
        push si
        
        jmp short bypass
ss_sav  dw ?
sp_sav  dw ?        
bypass:
        push ss_sav
        push sp_sav
                
        mov ss_sav, ss
        mov sp_sav, sp
        
        lea ax, callwithpsp_ret
        lds bx, address
        les cx, psp
        
        mov cx, ss_new
        mov si, sp_new
        cli
        mov ss, cx
        mov sp, si
        sti
        
        push cs
        push ax

; MSDOS pushes a 0 on the stack to do a near return, which points
; to the PSP, which has an int 20h in it, which is terminate, but
; we leave that for the caller to organize instead so that we can
; do a retf if we want.
        
;        mov ax, 0
;        push ax
                
        push ds
        push bx

        push es
        pop ds

        mov ax, 0ffffh
        
        retf    ; call desired routine

callwithbypass:
        pop ax ; skip return address,
        pop ax ;    won't be needed
        pop ax ; get the return code

callwithpsp_ret:

        cli
        mov ss, ss_sav
        mov sp, sp_sav
        sti
        
        pop sp_sav
        pop ss_sav

        pop si        
        pop es
        pop cx
        pop ds
        pop bx
        ret
callwithpsp endp

ifdef NEED_HANDLER
instint proc,
        uses bx, es
        push 0
        pop es
        cli
        mov bx, offset handler
        mov es:[84h], bx
        mov bx, seg handler
        mov es:[86h], bx
        sti
        ret
instint endp
endif

ifdef NEED_HANDLER
handler proc
        push ax
        push ax   ; dummy, actually cflag storage
        push bx
        push cx
        push dx
        push si
        push di
        push ds
        push es

        mov dx, DGROUP
        mov ds, dx
        mov ax, sp
        push ss
        push ax
        call int21
        add sp, 4
        
        pop es
        pop ds
        pop di
        pop si
        pop dx
        pop cx
        pop bx
        pop ax   ; actually cflag

        cmp ax, 0
        je clear
        jmp notclear
clear:
        pop ax
        push bp
        mov bp, sp
        and word ptr [bp+6],0fffeh
        pop bp
        iret
notclear:        
        pop ax
        push bp
        mov bp, sp
        or word ptr [bp+6],0001h
        pop bp
        iret
handler endp
endif        

       
; enable a20 line
; original from Matthew Parker
; mods by Paul Edwards

a20e proc
a20e_ii:
        in al, 064h
        test al, 2
        jnz a20e_ii
        mov al, 0dfh
        out 060h, al
        ret
a20e endp

reboot proc
        push es
        mov ax, 040h
        mov es, ax
        mov word ptr es:[072h], 01234h
        
; we want to do a jmp 0ffffh:0000h, but that seems not to assemble
        mov ax, 0ffffh
        push ax
        mov ax, 0
        push ax
        retf
        ret
reboot endp

getabs proc, address3: dword
        push ds
        push bx
        lds bx,address3
        mov ax, ds
        shl ax, 12
        mov ds, ax
        mov ah,0
        mov al,[bx]
        pop bx
        pop ds
        ret
getabs endp

putabs proc, address4: dword, character: word
        push ds
        push bx
        lds bx,address4
        mov ax, ds
        shl ax, 12
        mov ds, ax        
        mov ax, character
        mov [bx],al
        pop bx
        pop ds
        ret
putabs endp

end
