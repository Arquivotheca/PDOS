; Released to the public domain by Matthew Parker on 4 December 1995
; Mods by Paul Edwards, also public domain.
; For different models just change the .model directive

% .model memodel, c

extrn int21:proc
public instint, handler

        .code

instint proc uses bx es
        mov bx, 0
        push bx
        pop es
        cli
        mov bx, offset handler
        mov es:[84h], bx
        mov bx, seg handler
        mov es:[86h], bx
        sti
        ret
instint endp

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

end
