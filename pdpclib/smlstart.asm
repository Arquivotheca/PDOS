;
; Released to the public domain by Alexey Frunze.
;

; File: c0dh_jw.asm
;
; Attempt to adapt Smaller C's c0dh.asm from NASM to JWASM.

.model large
.386

    extern ___start__:byte
    extern __start__relot:byte, __stop__relot:byte
    extern __start__relod:byte, __stop__relod:byte
    extern __start__bss:byte, __stop__bss:byte

text segment "CODE"

    public __start
__start:

    ; perform code/data relocations

    call    labnext
labnext:
    xor     ebx, ebx
    mov     bx, cs
    shl     ebx, 4
    xor     eax, eax
    pop     ax
    add     ebx, eax
    sub     ebx, offset labnext ; ebx = base physical address

    mov     esi, offset __start__relot
relo_text_loop:
    cmp     esi, offset __stop__relot
    jae     relo_text_done

    lea     edi, [ebx + esi] ; edi = physical address of a relocation table element

    ror     edi, 4
    mov     ds, di
    shr     edi, 28

    mov     edi, [di]
    add     edi, ebx ; edi = physical address of a dword to which to add ebx and which then to transform into seg:ofs far pointer

    ror     edi, 4
    mov     ds, di
    shr     edi, 28

    mov     eax, [di]
    add     eax, ebx

    shl     eax, 12
    rol     ax, 4

    mov     [di], eax

    add     esi, 4
    jmp     relo_text_loop
relo_text_done:

    mov     esi, offset __start__relod
relo_data_loop:
    cmp     esi, offset __stop__relod
    jae     relo_data_done

    lea     edi, [ebx + esi] ; edi = physical address of a relocation table element

    ror     edi, 4
    mov     ds, di
    shr     edi, 28

    mov     edi, [di]
    add     edi, ebx ; edi = physical address of a dword to which to add ebx

    ror     edi, 4
    mov     ds, di
    shr     edi, 28

    add     [di], ebx ; actual relocation

    add     esi, 4
    jmp     relo_data_loop
relo_data_done:

    ; Init .bss

    push    ebx

    lea     edi, [ebx + offset __start__bss]
    lea     ebx, [ebx + offset __stop__bss]
    sub     ebx, edi
    ror     edi, 4
    mov     es, di
    shr     edi, 28
    xor     al, al
    cld

bss1:
    mov     ecx, 32768
    cmp     ebx, ecx
    jc      bss2

    sub     ebx, ecx
    rep     stosb
    and     di, 15
    mov     si, es
    add     si, 2048
    mov     es, si
    jmp     bss1

bss2:
    mov     cx, bx
    rep     stosb

    pop     ebx

    lea     eax, [ebx + offset ___start__]
    shl     eax, 12
    rol     ax, 4
    push    eax
    retf        ; __start__() will set up argc and argv for main() and call exit(main(argc, argv))

rt:
    dd      offset rt
text ends

_relot segment dword "CONST"
    dd      offset rt ; .relot must exist for __start__relot and __stop__relot to also exist
_relot ends

data segment "DATA"
rd:
    dd      offset rd
data ends

_relod segment dword "CONST"
    dd      offset rd ; .relod must exist for __start__relod and __stop__relod to also exist
_relod ends

_bss segment "BSS"
    ; .bss must exist for __start__bss and __stop__bss to also exist
    dd      ?
_bss ends

end
