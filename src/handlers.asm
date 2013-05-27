; Released to the public domain by Matthew Parker on 4 December 1995
; Mods by Paul Edwards, also public domain.
; For different models just change the .model directive

% .model memodel, c

public instint

extrn int20:proc
extrn int21:proc
extrn int25:proc

public handler20
public handler21
public handler25

        .code

instint proc uses bx es
        mov bx, 0
        push bx
        pop es
        cli
        mov bx, offset handler20
        mov es:[80h], bx
        mov bx, seg handler20
        mov es:[82h], bx
        mov bx, offset handler21
        mov es:[84h], bx
        mov bx, seg handler21
        mov es:[86h], bx
        mov bx, offset handler25
        mov es:[94h], bx
        mov bx, seg handler25
        mov es:[96h], bx
        sti
        ret
instint endp

handler20 proc
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
        call int20
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
        je clear21
        jmp notclear21
clear20:
        pop ax
        push bp
        mov bp, sp
        and word ptr [bp+6],0fffeh
        pop bp
        iret
notclear20:
        pop ax
        push bp
        mov bp, sp
        or word ptr [bp+6],0001h
        pop bp
        iret
handler20 endp

handler21 proc
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
        je clear21
        jmp notclear21
clear21:
        pop ax
        push bp
        mov bp, sp
        and word ptr [bp+6],0fffeh
        pop bp
        iret
notclear21:
        pop ax
        push bp
        mov bp, sp
        or word ptr [bp+6],0001h
        pop bp
        iret
handler21 endp

handler25 proc
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
        call int25
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
        je clear25
        jmp notclear25
clear25:
        pop ax
        push bp
        mov bp, sp
        and word ptr [bp+6],0fffeh
        pop bp
        iret
notclear25:
        pop ax
        push bp
        mov bp, sp
        or word ptr [bp+6],0001h
        pop bp
        iret
handler25 endp

end
