/ startup code for 32 bit version of pdos
/ written by Paul Edwards
/ released to the public domain

/ symbols defined outside of here that are accessed
        .globl _gotint
        .globl saveesp

/ symbols defined here that are accessed from elsewhere
        .globl _inthdlr
        .globl _inthdlr_8
        .globl _inthdlr_9
        .globl _inthdlr_10
        .globl _inthdlr_13
        .globl _inthdlr_15
        .globl _inthdlr_16
        .globl _inthdlr_1A
        .globl _inthdlr_20
        .globl _inthdlr_21
        .globl _inthdlr_25
        .globl _inthdlr_26
        .globl _int_enable

        .text

/////////////////////////////////////////////////////////////
/ void inthdlr(void);
/
/ handling interrupts is very complex.  here is an example:
/
/ command.com, 0x30 does int 21 to exec pgm world
/ interrupt saves esp into saveesp
/ then a load is done, clobbering saveesp, but not before saving it,
/ although since it was already 0x10 it doesn't need to be saved in this case
/ then world is executed, which does an int 21 to do a print
/ it saves the old saveesp onto the stack, puts the new esp into saveesp,
/ then there is a bios call, but it doesn't do anything since ss is already
/ 0x10.  then the interrupt ends, restoring saveesp

/ _inthdlr is the default interrupt handler designed to do nothing.
/ It sets the interrupt number to 0 for recognition by gotint.
_inthdlr:
        push   %eax
        mov    %ds, %ax
        push   %eax
        mov    $0x10, %eax
        mov    %ax, %ds
        push   intnum
        movl   $0x0, intnum
        jmp    _inthdlr_q
_inthdlr_8:
        push   %eax
        mov    %ds, %ax
        push   %eax
        mov    $0x10, %eax
        mov    %ax, %ds
        push   intnum
        movl   $0x8, intnum
        jmp    _inthdlr_q
_inthdlr_9:
        push   %eax
        mov    %ds, %ax
        push   %eax
        mov    $0x10, %eax
        mov    %ax, %ds
        push   intnum
        movl   $0x9, intnum
        jmp    _inthdlr_p
_inthdlr_10:
        push   %eax
        mov    %ds, %ax
        push   %eax
        mov    $0x10, %eax
        mov    %ax, %ds
        push   intnum
        movl   $0x10, intnum
        jmp    _inthdlr_p
_inthdlr_13:
        push   %eax
        mov    %ds, %ax
        push   %eax
        mov    $0x10, %eax
        mov    %ax, %ds
        push   intnum
        movl   $0x13, intnum
        jmp    _inthdlr_p
_inthdlr_15:
        push   %eax
        mov    %ds, %ax
        push   %eax
        mov    $0x10, %eax
        mov    %ax, %ds
        push   intnum
        movl   $0x15, intnum
        jmp    _inthdlr_p
_inthdlr_16:
        push   %eax
        mov    %ds, %ax
        push   %eax
        mov    $0x10, %eax
        mov    %ax, %ds
        push   intnum
        movl   $0x16, intnum
        jmp    _inthdlr_p
_inthdlr_1A:
        push   %eax
        mov    %ds, %ax
        push   %eax
        mov    $0x10, %eax
        mov    %ax, %ds
        push   intnum
        movl   $0x1A, intnum
        jmp    _inthdlr_p
_inthdlr_20:
        push   %eax
        mov    %ds, %ax
        push   %eax
        mov    $0x10, %eax
        mov    %ax, %ds
        push   intnum
        movl   $0x20, intnum
        jmp    _inthdlr_p
_inthdlr_21:
        push   %eax
        mov    %ds, %ax
        push   %eax
        mov    $0x10, %eax
        mov    %ax, %ds
        push   intnum
        movl   $0x21, intnum
        jmp    _inthdlr_p
_inthdlr_25:
        push   %eax
        mov    %ds, %ax
        push   %eax
        mov    $0x10, %eax
        mov    %ax, %ds
        push   intnum
        movl   $0x25, intnum
        jmp    _inthdlr_p
_inthdlr_26:
        push   %eax
        mov    %ds, %ax
        push   %eax
        mov    $0x10, %eax
        mov    %ax, %ds
        push   intnum
        movl   $0x26, intnum
        jmp    _inthdlr_p
        
/ by the time we get here, the following things are on the stack:
/ original eax, original ds (stored as doubleword), original intnum

_inthdlr_p:
        push   saveess
        push   saveesp
        push   %ebx
        push   saveeax
        push   saveebx
        mov    $0, %eax
        mov    %ss, %ax
        mov    %eax, saveess
        mov    %esp, %eax
        mov    %eax, saveesp
        push   %ebp
        mov    %esp, %ebp        
/ Restore original eax (at time of interrupt) which is now located
/ at offset 32 thanks to the above pushes
        mov    32(%ebp), %eax
        mov    %eax, saveeax
        pop    %ebp
        cmp    $0x10, saveess
        je     level10        
        mov    $0x10, %eax
        mov    %ax, %ss
        mov    %ax, %es
        mov    %ax, %fs
        mov    %ax, %gs
        mov    call32_esp, %eax
        mov    %eax, %esp
level10:
        mov    saveeax, %eax
        push   %edx
/ above is actually room for flags
        push   %edx
/ above is actually room for cflag
        push   %edi
        push   %esi
        push   %edx
        push   %ecx
        push   %ebx
        push   %eax
        mov    %eax, %esi
        mov    %ebx, %edi
        mov    %esp, %eax
/ above is pointer to saved registers
        push   %eax
        mov    intnum, %edx
        push   %edx
/ above interrupt number
        call   _gotint
        pop    %edx
        pop    %eax
/  signal pic to reenable interrupts
        mov    $0x20, %dx
        mov    $0x20, %al
        outb   %al, %dx
        pop    %eax
        pop    %ebx
        pop    %ecx
        pop    %edx
        pop    %esi
        pop    %edi
        mov    %eax, saveeax
        pop    %eax
/ above is actually cflag
        mov    %ebx, saveebx
        pop    %ebx
/ above is actually flags
        cmp    $0, %eax
/ +++ We shouldn't actually need these different paths for
/ carry set and clear, but for some reason the flags do
/ not appear to have carry set properly, but the carry
/ flag value is set properly
        je     clear
        jmp    notclear
clear:
        cmp    $0x10, saveess
        je     level10_c
        mov    saveesp, %eax
        mov    %eax, %esp
level10_c:
        mov    saveess, %eax
        mov    %ax, %ss
        push   %ebp
        mov    %esp, %ebp
        mov    %bl, 44(%ebp)
        mov    saveebx, %ebx
        mov    %ebx, 12(%ebp)
        push   %eax
        mov    saveeax, %eax
        mov    %eax, 32(%ebp)
        pop    %eax
        pop    %ebp
        mov    %ax, %es
        mov    %ax, %fs
        mov    %ax, %gs
        pop    saveebx
        pop    saveeax
        pop    %ebx
        pop    saveesp
        pop    saveess
        pop    intnum
        pop    %eax
        mov    %ax, %ds
        pop    %eax
        push   %ebp
        mov    %esp, %ebp
        and    $0xfffffffe, 12(%ebp)
        pop    %ebp
        iret        
notclear:
        cmp    $0x10, saveess
        je     level10_nc
        mov    saveesp, %eax
        mov    %eax, %esp
level10_nc:
        mov    saveess, %eax
        mov    %ax, %ss
        push   %ebp
        mov    %esp, %ebp
        mov    %bl, 44(%ebp)
        mov    saveebx, %ebx
        mov    %ebx, 12(%ebp)
        push   %eax
        mov    saveeax, %eax
        mov    %eax, 32(%ebp)
        pop    %eax
        pop    %ebp
        mov    %ax, %es
        mov    %ax, %fs
        mov    %ax, %gs
        pop    saveebx
        pop    saveeax
        pop    %ebx
        pop    saveesp
        pop    saveess
        pop    intnum
        pop    %eax
        mov    %ax, %ds
        pop    %eax
        push   %ebp
        mov    %esp, %ebp
        or     $1, 12(%ebp)
        pop    %ebp
        iret        

/ This is for interrupts that should not alter
/ the flags, like the timer interrupt
_inthdlr_q:
        push   saveess
        push   saveesp
        push   %ebx
        push   saveeax
        push   saveebx
        mov    $0, %eax
        mov    %ss, %ax
        mov    %eax, saveess
        mov    %esp, %eax
        mov    %eax, saveesp
        push   %ebp
        mov    %esp, %ebp
/ Restore original eax (at time of interrupt) which is now located
/ at offset 32 thanks to the above pushes
        mov    32(%ebp), %eax
        mov    %eax, saveeax
        pop    %ebp
        cmp    $0x10, saveess
        je     level10b
        mov    $0x10, %eax
        mov    %ax, %ss
        mov    %ax, %es
        mov    %ax, %fs
        mov    %ax, %gs
        mov    call32_esp, %eax
        mov    %eax, %esp
level10b:
        mov    saveeax, %eax
        push   %edx
/ above is actually room for flags
        push   %edx
/ above is actually room for cflag
        push   %edi
        push   %esi
        push   %edx
        push   %ecx
        push   %ebx
        push   %eax
        mov    %eax, %esi
        mov    %ebx, %edi
        mov    %esp, %eax
/ above is pointer to saved registers
        push   %eax
        mov    intnum, %edx
        push   %edx
/ above interrupt number
        call   _gotint
        pop    %edx
        pop    %eax
/  signal pic to reenable interrupts
        mov    $0x20, %dx
        mov    $0x20, %al
        outb   %al, %dx
        pop    %eax
        pop    %ebx
        pop    %ecx
        pop    %edx
        pop    %esi
        pop    %edi
        mov    %eax, saveeax
        pop    %eax
/ above is actually cflag
        mov    %ebx, saveebx
        pop    %ebx
/ above is actually flags
        cmp    $0x10, saveess
        je     level10_cb
        mov    saveesp, %eax
        mov    %eax, %esp
level10_cb:
        mov    saveess, %eax
        mov    %ax, %ss
        push   %ebp
        mov    %esp, %ebp
/ Don't set the flags
/        mov    %bl, 44(%ebp)
        mov    saveebx, %ebx
        mov    %ebx, 12(%ebp)
        push   %eax
        mov    saveeax, %eax
        mov    %eax, 32(%ebp)
        pop    %eax
        pop    %ebp
        mov    %ax, %es
        mov    %ax, %fs
        mov    %ax, %gs
        pop    saveebx
        pop    saveeax
        pop    %ebx
        pop    saveesp
        pop    saveess
        pop    intnum
        pop    %eax
        mov    %ax, %ds
        pop    %eax
        push   %ebp
        mov    %esp, %ebp
/ Don't alter carry bit
/        and    $0xfffffffe, 12(%ebp)
        pop    %ebp
        iret

/////////////////////////////////////////////////////////////
/ void int_enable(void);
/
_int_enable:
        sti
        ret

.data
        .p2align 2
saveeax:
        .space 4
        .p2align 2
saveebx:
        .space 4
        .p2align 2
saveess:
        .space 4
        .p2align 2
intnum:
        .space 4
