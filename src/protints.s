/ protected mode interrupt handlers
/ written by Paul Edwards
/ released to the public domain

/ symbols defined outside of here that are accessed
        .globl _gotint

/ symbols defined here that are accessed from elsewhere
        .globl _inthdlr
        .globl _inthdlr_D
        .globl _inthdlr_E
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
        .globl _inthdlr_BE
        .globl _int_enable

        .text

/////////////////////////////////////////////////////////////
/ void inthdlr_??(void);

/ Protected mode exceptions
_inthdlr_D:
        pushl   $0xD
        jmp    _inthdlr_r
_inthdlr_E:
        pushl   $0xE
        jmp    _inthdlr_r
/ System calls
_inthdlr_20:
        pushl   $0x20
        jmp    _inthdlr_p
_inthdlr_21:
        pushl   $0x21
        jmp    _inthdlr_p
_inthdlr_25:
        pushl   $0x25
        jmp    _inthdlr_p
_inthdlr_26:
        pushl   $0x26
        jmp    _inthdlr_p
_inthdlr_80:
        pushl   $0x80
        jmp    _inthdlr_p
/ Interrupt handlers used to access BIOS
_inthdlr_A0:
        pushl   $0xA0
        jmp    _inthdlr_p
_inthdlr_A3:
        pushl   $0xA3
        jmp    _inthdlr_p
_inthdlr_A5:
        pushl   $0xA5
        jmp    _inthdlr_p
_inthdlr_A6:
        pushl   $0xA6
        jmp    _inthdlr_p
_inthdlr_AA:
        pushl   $0xAA
        jmp    _inthdlr_p
/ IRQ handlers.
_inthdlr_B0:
        pushl   $0xB0
        jmp    _inthdlr_q
_inthdlr_B1:
        pushl   $0xB1
        jmp    _inthdlr_q
_inthdlr_BE:
        pushl   $0xBE
        jmp    _inthdlr_q

/ by the time we get here, the following things are on the stack:
/ old SS, old ESP, EFLAGS, old CS, return address, (error code),
/ interrupt number

/ For interrupts modifying the flags, like system calls
_inthdlr_p:
        push   %eax
/ above is original eax
        mov    %ds, %ax
        push   %eax
        mov    $0x10, %eax
        mov    %ax, %ds
        mov    %ax, %es
        mov    %ax, %fs
        mov    %ax, %gs
/ above saves original ds and switches segments
        push   %ebp
        mov    %esp, %ebp
        mov    8(%ebp), %eax
        pop    %ebp
        subl   $4, %esp
/ above is actually room for flags
        subl   $4, %esp
/ above is actually room for cflag
        push   %edi
        push   %esi
        push   %edx
        push   %ecx
        push   %ebx
        push   %eax
        mov    %esp, %eax
        push   %eax
/ above is pointer to saved registers
        push   %ebp
        mov    %esp, %ebp
        mov    48(%ebp), %edx
        pop    %ebp
        push   %edx
/ above is interrupt number copied for _gotint
        call   _gotint
        pop    %edx
        pop    %eax
/ pops saved registers
        pop    %eax
        push   %ebp
        mov    %esp, %ebp
        mov    %eax, 36(%ebp)
        pop    %ebp
/ above saves new eax on the stack where original eax was
/ so the register can be used freely
        pop    %ebx
        pop    %ecx
        pop    %edx
        pop    %esi
        pop    %edi
        addl   $4, %esp
/ above is cflag, handled by _gotint
        pop    %eax
/ above is flags
        push   %ebp
        mov    %esp, %ebp
/ update the bottom 8 bits plus bit 11 (OF) of the flags
        andl   $0xfffff700, 24(%ebp)
        and    $0x8ff, %eax
        or     %eax, 24(%ebp)
        pop    %ebp
/ restores segment registers
        pop    %eax
        mov    %ax, %ds
        mov    %ax, %es
        mov    %ax, %fs
        mov    %ax, %gs
/ gets new eax from the stack
        pop    %eax
        addl   $4, %esp
/ above is original interrupt number
        iret

/ This is for interrupts that should not alter
/ the flags, like the timer interrupt
_inthdlr_q:
        push   %eax
/ above is original eax
        mov    %ds, %ax
        push   %eax
        mov    $0x10, %eax
        mov    %ax, %ds
        mov    %ax, %es
        mov    %ax, %fs
        mov    %ax, %gs
/ above saves original ds and switches segments
        push   %ebp
        mov    %esp, %ebp
        mov    8(%ebp), %eax
        pop    %ebp
        subl   $4, %esp
/ above is actually room for flags
        subl   $4, %esp
/ above is actually room for cflag
        push   %edi
        push   %esi
        push   %edx
        push   %ecx
        push   %ebx
        push   %eax
        mov    %esp, %eax
        push   %eax
/ above is pointer to saved registers
        push   %ebp
        mov    %esp, %ebp
        mov    48(%ebp), %edx
        pop    %ebp
        push   %edx
/ above is interrupt number copied for _gotint
        call   _gotint
        pop    %edx
        pop    %eax
/ pops saved registers
        pop    %eax
        push   %ebp
        mov    %esp, %ebp
        mov    %eax, 36(%ebp)
        pop    %ebp
/ above saves new eax on the stack where original eax was
/ so the register can be used freely
        pop    %ebx
        pop    %ecx
        pop    %edx
        pop    %esi
        pop    %edi
        addl   $4, %esp
/ above is cflag, handled by _gotint
        addl   $4, %esp
/ above is flags, ignored
/ restores segment registers
        pop    %eax
        mov    %ax, %ds
        mov    %ax, %es
        mov    %ax, %fs
        mov    %ax, %gs
/ gets new eax from the stack
        pop    %eax
        addl   $4, %esp
/ above is original interrupt number
        iret

/ This is for exceptions that have an error code pushed when they occur.
_inthdlr_r:
        push   %eax
/ above is original eax
        mov    %ds, %ax
        push   %eax
        mov    $0x10, %eax
        mov    %ax, %ds
        mov    %ax, %es
        mov    %ax, %fs
        mov    %ax, %gs
/ above saves original ds and switches segments
        push   %ebp
        mov    %esp, %ebp
        mov    16(%ebp), %eax
        pop    %ebp
        push   %eax
/ above is duplicated error code so it can be accessed as regs[8]
        push   %ebp
        mov    %esp, %ebp
        mov    12(%ebp), %eax
        pop    %ebp
        subl   $4, %esp
/ above is actually room for flags
        subl   $4, %esp
/ above is actually room for cflag
        push   %edi
        push   %esi
        push   %edx
        push   %ecx
        push   %ebx
        push   %eax
        mov    %esp, %eax
        push   %eax
/ above is pointer to saved registers
        push   %ebp
        mov    %esp, %ebp
        mov    52(%ebp), %edx
        pop    %ebp
        push   %edx
/ above is interrupt number copied for _gotint
        call   _gotint
        pop    %edx
        pop    %eax
/ pops saved registers
        pop    %eax
        push   %ebp
        mov    %esp, %ebp
        mov    %eax, 40(%ebp)
        pop    %ebp
/ above saves new eax on the stack where original eax was
/ so the register can be used freely
        pop    %ebx
        pop    %ecx
        pop    %edx
        pop    %esi
        pop    %edi
        addl   $4, %esp
/ above is cflag, handled by _gotint
        addl   $4, %esp
/ above is flags, ignored
        addl   $4, %esp
/ above is duplicated error code
/ restores segment registers
        pop    %eax
        mov    %ax, %ds
        mov    %ax, %es
        mov    %ax, %fs
        mov    %ax, %gs
/ gets new eax from the stack
        pop    %eax
        addl   $4, %esp
/ above is original interrupt number
        addl   $4, %esp
/ above is original error code
        iret

/////////////////////////////////////////////////////////////
/ void int_enable(void);
/
_int_enable:
        sti
        ret
