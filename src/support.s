/ support routines
/ written by Paul Edwards
/ released to the public domain

        .globl _int86
        .globl _int86x

        .text

/ Because of the C calling convention, and the fact that the seg
/ regs are the last parameter, and they're not actually used, the
/ entry point for _int86 can be reused for _int86x
_int86x:
_int86:
        push    %ebp
        mov     %esp, %ebp
        push    %eax
        push    %ebx
        push    %ecx
        push    %edx
        push    %esi
        push    %edi

        mov     12(%ebp), %esi
        mov     0(%esi), %eax
        mov     4(%esi), %ebx
        mov     8(%esi), %ecx
        mov     12(%esi), %edx
        mov     20(%esi), %edi
        mov     16(%esi), %esi
        
        cmp     $0x10, 8(%ebp)
        jne     not10
        int     $0x10
        jmp     fintry
not10:

        cmp     $0x13, 8(%ebp)
        jne     not13
        int     $0x13
        jmp     fintry
not13:

        cmp     $0x15, 8(%ebp)
        jne     not15
        int     $0x15
        jmp     fintry
not15:

        cmp     $0x16, 8(%ebp)
        jne     not16
        int     $0x16
        jmp     fintry
not16:

        cmp     $0x21, 8(%ebp)
        jne     not21
        int     $0x21
        jmp     fintry
not21:

fintry:
        push    %esi
        mov     16(%ebp), %esi
        mov     %eax, 0(%esi)
        mov     %ebx, 4(%esi)
        mov     %ecx, 8(%esi)
        mov     %edx, 12(%esi)
        mov     %edi, 20(%esi)
        pop     %eax  / actually esi
        mov     %eax, 16(%esi)
        mov     $0, %eax
        mov     %eax, 24(%esi)
        jnc     nocarry
        mov     $1, %eax
        mov     %eax, 24(%esi)
nocarry:        

        pop     %edi
        pop     %esi
        pop     %edx
        pop     %ecx
        pop     %ebx
        pop     %eax
        pop     %ebp
        ret
