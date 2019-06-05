/ protected mode interrupt handlers
/ written by Paul Edwards
/ released to the public domain

/ symbols defined outside of here that are accessed
        .globl _gotint
        .globl saveesp

/ symbols defined here that are accessed from elsewhere
        .globl _inthdlr
        .globl _inthdlr_8
        .globl _inthdlr_9
        .globl _inthdlr_E
        .globl _inthdlr_10
        .globl _inthdlr_13
        .globl _inthdlr_15
        .globl _inthdlr_16
        .globl _inthdlr_1A
        .globl _inthdlr_20
        .globl _inthdlr_21
        .globl _inthdlr_25
        .globl _inthdlr_26
        .globl _inthdlr_80
        .globl _inthdlr_A0
        .globl _inthdlr_A3
        .globl _inthdlr_A5
        .globl _inthdlr_A6
        .globl _inthdlr_AA
        .globl _inthdlr_B0
        .globl _inthdlr_B1
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
        jmp    _inthdlr_q
_inthdlr_E:
        push   %eax
        mov    %ds, %ax
        push   %eax
        mov    $0x10, %eax
        mov    %ax, %ds
        push   intnum
        movl   $0xE, intnum
        jmp    _inthdlr_r
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
_inthdlr_80:
        push   %eax
        mov    %ds, %ax
        push   %eax
        mov    $0x10, %eax
        mov    %ax, %ds
        push   intnum
        movl   $0x80, intnum
        jmp    _inthdlr_p
/ Interrupt handlers used to access BIOS
_inthdlr_A0:
        push   %eax
        mov    %ds, %ax
        push   %eax
        mov    $0x10, %eax
        mov    %ax, %ds
        push   intnum
        movl   $0xA0, intnum
        jmp    _inthdlr_p
_inthdlr_A3:
        push   %eax
        mov    %ds, %ax
        push   %eax
        mov    $0x10, %eax
        mov    %ax, %ds
        push   intnum
        movl   $0xA3, intnum
        jmp    _inthdlr_p
_inthdlr_A5:
        push   %eax
        mov    %ds, %ax
        push   %eax
        mov    $0x10, %eax
        mov    %ax, %ds
        push   intnum
        movl   $0xA5, intnum
        jmp    _inthdlr_p
_inthdlr_A6:
        push   %eax
        mov    %ds, %ax
        push   %eax
        mov    $0x10, %eax
        mov    %ax, %ds
        push   intnum
        movl   $0xA6, intnum
        jmp    _inthdlr_p
_inthdlr_AA:
        push   %eax
        mov    %ds, %ax
        push   %eax
        mov    $0x10, %eax
        mov    %ax, %ds
        push   intnum
        movl   $0xAA, intnum
        jmp    _inthdlr_p
/ IRQ handlers.
_inthdlr_B0:
        push   %eax
        mov    %ds, %ax
        push   %eax
        mov    $0x10, %eax
        mov    %ax, %ds
        push   intnum
        movl   $0xB0, intnum
        jmp    _inthdlr_q
_inthdlr_B1:
        push   %eax
        mov    %ds, %ax
        push   %eax
        mov    $0x10, %eax
        mov    %ax, %ds
        push   intnum
        movl   $0xB1, intnum
        jmp    _inthdlr_q
        
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
/ saveess and saveesp must be saved on the stack
/ because task switch can occur
/ and the next interrupt might not restore them
/ before switch back happens
        push   saveess
        push   saveesp
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
/ pops saved registers
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
        pop    saveesp
        pop    saveess
/ above are saved saveesp and saveess to handle task switches
        cmp    $0x10, saveess
        je     level10b
        mov    saveesp, %eax
        mov    %eax, %esp
level10b:
        mov    saveess, %eax
        mov    %ax, %ss
        push   %ebp
        mov    %esp, %ebp
/ update the bottom 8 bits plus bit 11 (OF) of the flags
        andl   $0xfffff700, 44(%ebp)
        and    $0x8ff, %ebx
        or     %ebx, 44(%ebp)
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
        je     level10c
        mov    $0x10, %eax
        mov    %ax, %ss
        mov    %ax, %es
        mov    %ax, %fs
        mov    %ax, %gs
        mov    call32_esp, %eax
        mov    %eax, %esp
level10c:
        mov    saveeax, %eax
/ saveess and saveesp must be saved on stack
/ because task switch can occur
/ and the next interrupt might not restore them
/ before switch back happens
        push   saveess
        push   saveesp
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
/ pops saved registers
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
        pop    saveesp
        pop    saveess
/ above are saved saveesp and saveess to handle task switches
        cmp    $0x10, saveess
        je     level10d
        mov    saveesp, %eax
        mov    %eax, %esp
level10d:
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
        pop    %ebp
        iret

/ This is for exceptions that have an error code pushed when they occur.
_inthdlr_r:
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
/ Saves the error code pushed after the data for iret.
        mov    36(%ebp), %eax
        mov    %eax, saveerrorcode
        pop    %ebp
        cmp    $0x10, saveess
        je     level10e
        mov    $0x10, %eax
        mov    %ax, %ss
        mov    %ax, %es
        mov    %ax, %fs
        mov    %ax, %gs
        mov    call32_esp, %eax
        mov    %eax, %esp
level10e:
        mov    saveeax, %eax
/ saveess and saveesp must be saved on stack
/ because task switch can occur
/ and the next interrupt might not restore them
/ before switch back happens
        push   saveess
        push   saveesp
        push   saveerrorcode
/ above is duplicated error code
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
/ pops saved registers
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
        addl   $4, %esp
/ above is the duplicated error code
        pop    saveesp
        pop    saveess
/ above are saved saveesp and saveess to handle task switches
        cmp    $0x10, saveess
        je     level10f
        mov    saveesp, %eax
        mov    %eax, %esp
level10f:
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
        pop    %ebp
/ Removes the error code pushed when the exception occured.
        addl   $4, %esp
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
saveerrorcode:
        .space 4
        .p2align 2
intnum:
        .space 4
