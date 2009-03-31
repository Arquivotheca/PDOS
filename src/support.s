/ support routines
/ written by Paul Edwards
/ released to the public domain

        .globl _int86
        .globl _int86x
        .globl _enable
        .globl _disable
        .globl __setj
        .globl __longj

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

        cmp     $0x20, 8(%ebp)
        jne     not20
        int     $0x20
        jmp     fintry
not20:

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

/ this is actually esi
        pop     %eax
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

_enable:
        sti
        ret

_disable:
        cli
        ret


/ setjmp and longjmp are untested under PDOS32 due to the
/ test environment currently not being available

__setj:
        push    %ebp
        mov     %esp, %ebp
        mov     12(%ebp), %eax
        push    %ebx
        mov     %esp, %ebx

/ really esp
        push    %ebx
        
        mov     %ecx, 4(%eax)
        mov     %edx, 8(%eax)
        mov     %edi, 12(%eax)
        mov     %esi, 16(%eax)
        
        pop     %ebx
/ really esp
        mov     %ebx, 20(%eax)
        mov     0(%ebp), %ebx
/ really ebp
        mov     %ebx, 24(%eax)

/ return address
        mov     4(%ebp), %ebx
        mov     %ebx, 20(%eax)

        pop     %ebx
        mov     %ebx, 0(%eax)
        mov     0, %eax

        pop     %ebp        
        ret


__longj:
        push    %ebp
        mov     %esp, %ebp
        mov     12(%ebp), %eax
        mov     20(%eax), %ebp
        mov     %ebp, %esp

/ position of old ebx
        pop     %ebx
/ position of old ebp
        pop     %ebx
/ position of old return address
        pop     %ebx

/ return address
        mov     28(%eax), %ebx
        push    %ebx

/ ebp saved as normal
        mov     24(%eax), %ebx
        push    %ebx
        mov     %esp, %ebp
        
        mov     0(%eax), %ebx
        mov     4(%eax), %ecx
        mov     8(%eax), %edx
        mov     12(%eax), %edi
        mov     16(%eax), %esi

/ return value
        mov     32(%eax), %eax

        pop     %ebp
        ret
