/ pdos support routines in assembler
/ written by Paul Edwards
/ released to the public domain

/ symbols defined outside of here that are accessed
        .globl ___abscor
        .globl _subcor

/ symbols defined here that are accessed from elsewhere
        .globl _call32
        .globl _callwithbypass
        .globl _loadPageDirectory
        .globl _saveCR3
        .globl _enablePaging

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
/ +++ should next 2 %eax be %ax?
        mov    $0x8, %eax
        push   %eax
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
/ +++ should next 2 references be %ax instead of %eax?
        mov     $0x28, %eax
        push    %eax
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
/ +++ We need to test whether this should instead be
/ call *(%ebx)
        call    *%ebx
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
/ reenable interrupts
        sti

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

/////////////////////////////////////////////////////////////
/ void loadPageDirectory(unsigned long page_directory_address);
/ Loads Page Directory address into CR3.
_loadPageDirectory:
        push    %ebp
        mov     %esp, %ebp
        mov     8(%ebp), %eax
        mov     %eax, %cr3
        pop     %ebp
        ret

/////////////////////////////////////////////////////////////
/ unsigned long saveCR3(void);
/ Returns the content of CR3.
_saveCR3:
        push    %ebp
        mov     %esp, %ebp
        mov     %cr3, %eax
        pop     %ebp
        ret

/////////////////////////////////////////////////////////////
/ void enablePaging(void);
/ Sets the Paging bit of CR0.
_enablePaging:
        push    %ebp
        mov     %esp, %ebp
        mov     %cr0, %eax
        or      $0x80000000, %eax
        mov     %eax, %cr0
        pop     %ebp
        ret

.bss
        .p2align 2
saveess:
        .space 4
        .p2align 2
        .globl saveesp
saveesp:
        .space 4
        .p2align 2
saveesp2:
        .space 4
        .p2align 2
        .globl call32_esp
call32_esp:
        .space 4
