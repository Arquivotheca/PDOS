/ pdos support routines in assembler
/ written by Paul Edwards
/ released to the public domain

/ symbols defined outside of here that are accessed
        .globl ___abscor
        .globl _subcor

/ symbols defined here that are accessed from elsewhere
        .globl _call32
        .globl _callwithbypass

        .text

/////////////////////////////////////////////////////////////
/ int _call32(int entry, void *exeparms, int sp);
_call32:
        push    %ebp
        mov     %esp, %ebp
        pushf
/ disable interrupts so that we can play with stack        
/ Note - I think this can actually be moved down to where the
/ actual switch takes place
        cli
        push    %ebx
        push    %ecx
        push    %edx
        push    %esi
        push    %edi
        push    call32_esp
        push    saveesp2
        mov     %esp, call32_esp
/ save stack of caller
        mov     saveesp, %eax
        mov     %eax, saveesp2        
/ load abscor into edx
        mov    ___abscor, %edx
/ load return address into ecx
        lea    _call32_ret, %ecx
/ get subroutine's address into ebx
        mov    8(%ebp), %ebx
/ load address of single lret into edi
        lea    _call32_singlelret, %edi
        add    %edx, %edi
        sub    _subcor, %edi
/ load address of single ret into esi
        lea    _call32_singleret, %esi
        add    %edx, %esi
        sub    _subcor, %esi
/ load address of exe parm block into %edx
        mov    12(%ebp), %edx        
/ switch stack etc to new one
        mov    16(%ebp), %eax
/ I think this is where interrupts should really be disabled
        mov    %eax, %esp
        mov    $0x30, %ax
        mov    %ax, %ss
        mov    %ax, %gs
        mov    %ax, %fs
        mov    %ax, %es
        mov    %ax, %ds
        
/ push return address
        mov    $0x8, %ax
        push   %ax
        push   %ecx
/ push parameters for subroutine from right to left
        push   %edx
        mov    $0, %eax
        push   %eax
        push   %eax
        push   %eax        
/ push address of a "lret" statement so that they can come back
        push   %edi
/ push subroutine's address
        push    %ebx
/ push address of a "ret" statement so that we don't directly call
/ their code, but they can return from their executable with a
/ normal ret
        mov     $0x28, %ax
        push    %ax
        push    %esi
/ reenable interrupts for called program        
        sti
/ call it via far return        
        lret
_call32_singleret:
        ret
_call32_singlelret:
/ first we see if a callback is required
        push    %ebx
        mov     16(%esp), %ebx
        mov     20(%ebx), %ebx
        cmp     $0, %ebx
        je      no_callback
        push    %eax
        call    %ebx
        pop     %eax
no_callback:
        pop     %ebx
/ and skip the parameters too
        add     $16, %esp
        lret
_call32_ret:
/ disable interrupts so that we can play with stack
        cli
/ restore our old stack etc
        mov    $0x10, %bx
        mov    %bx, %ds
        mov    call32_esp, %esp
        mov    %bx, %ss
        mov    %bx, %gs
        mov    %bx, %fs
        mov    %bx, %es

_call32_pops:
        pop    saveesp2
        pop    call32_esp
        pop    %edi
        pop    %esi        
        pop    %edx
        pop    %ecx
        pop    %ebx
        popf
        pop    %ebp
        ret

                
/////////////////////////////////////////////////////////////
/ void _callwithbypass(int retcode);
/ for use by programs which terminate via int 21h.
/ This function works to get PDOS back to the state it was
/ when it called the user program.

_callwithbypass:
/ skip return address, not required
        pop     %eax
/ restore old esp
        mov     saveesp2, %eax
        mov     %eax, saveesp        
/ get return code
        pop     %eax
        jmp     _call32_ret


.data
        .comm saveess, 4
        .comm saveesp, 4
        .comm saveesp2, 4
        .comm call32_esp, 4

