/ pdos support routines in assembler
/ written by Paul Edwards
/ released to the public domain

/ symbols defined here that are accessed from elsewhere
        .globl _call32
        .globl _callwithbypass
        .globl _loadPageDirectory
        .globl _saveCR3
        .globl _enablePaging
        .globl _disablePaging
        .globl _readCR2
        .globl _switchFromToThread
        .globl _getEFLAGSAndDisable
        .globl _setEFLAGS
        .globl _callDllEntry
        .globl _loadTaskRegister

        .text

/////////////////////////////////////////////////////////////
/ int _call32(int entry, int sp, TCB *curTCB, unsigned int **esp0);
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
        mov     %esp, call32_esp
/ call32_esp has to be saved in TCB
        mov     16(%ebp), %edi
        mov     %esp, 4(%edi)
/ ESP0 in ringSwitchTSS is updated to handle ring switches
        mov     20(%ebp), %edi
        mov     %esp, 0(%edi)
/ get subroutine's address into ebx
        mov    8(%ebp), %ebx
/ I think this is where interrupts should really be disabled
        mov    $0x33, %ax
        mov    %ax, %gs
        mov    %ax, %fs
        mov    %ax, %es
        mov    %ax, %ds

/ New SS, new ESP, EFLAGS, new CS, entry point.
        push   $0x33
        mov    12(%ebp), %eax
        push   %eax
        pushf
/ Enables interrupts for the called program.
        pop    %eax
        or     $0x200, %eax
        push   %eax
        push   $0x2B
        push   %ebx
        iret

_call32_ret:
/ disable interrupts so that we can play with stack
        cli
/ restore our old stack
        mov    call32_esp, %esp
/ reenable interrupts
        sti

_call32_pops:
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
        addl   $4, %esp
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

/////////////////////////////////////////////////////////////
/ void disablePaging(void);
/ Clears the Paging bit of CR0.
_disablePaging:
        push    %ebp
        mov     %esp, %ebp
        mov     %cr0, %eax
        and     $0x7fffffff, %eax
        mov     %eax, %cr0
        pop     %ebp
        ret

/////////////////////////////////////////////////////////////
/ unsigned long readCR2(void);
/ Returns the content of CR2.
_readCR2:
        push    %ebp
        mov     %esp, %ebp
        mov     %cr2, %eax
        pop     %ebp
        ret

/////////////////////////////////////////////////////////////
/ void switchFromToThread(TCB *oldTCB, TCB *newTCB);
/ Switches from oldTCB thread to newTCB thread.
/ Interrupts should be disabled before calling.
_switchFromToThread:
/ Saves registers.
    push %eax
    push %ebx
    push %ecx
    push %edx
    push %esi
    push %edi
    push %ebp
    pushf

/ Loads oldTCB into EDI and newTCB into ESI.
    push %ebp
    mov %esp, %ebp
    mov 40(%ebp), %edi
    mov 44(%ebp), %esi
    pop %ebp

/ Saves ESP of the current thread into oldTCB.
    mov %esp, 0(%edi)

/ Loads state from newTCB.
    mov 0(%esi), %esp
    mov 4(%esi), %eax
    mov %eax, call32_esp

/ Code running after the switch.
/ Pops registers.
    popf
    pop %ebp
    pop %edi
    pop %esi
    pop %edx
    pop %ecx
    pop %ebx
    pop %eax
    ret

/////////////////////////////////////////////////////////////
/ unsigned int getEFLAGSAndDisable(void);
/ Returns current EFLAGS and disables interrupts.
_getEFLAGSAndDisable:
        push    %ebp
        mov     %esp, %ebp
        pushf
        pop     %eax
        cli
        pop     %ebp
        ret

/////////////////////////////////////////////////////////////
/ void setEFLAGS(unsigned int flags);
/ Sets EFLAGS.
_setEFLAGS:
        push    %ebp
        mov     %esp, %ebp
        mov     8(%ebp), %eax
        push    %eax
        popf
        pop     %ebp
        ret

/////////////////////////////////////////////////////////////
/ BOOL callDllEntry(void *entry_point, HINSTANCE hinstDll,
/                   DWORD fdwReason, LPVOID lpvReserved);
/ Calls the provided DLL entry point using __stdcall convention.
_callDllEntry:
        push    %ebp
        mov     %esp, %ebp
/ Copies arguments for the DLL entry function (right to left).
        mov     20(%ebp), %eax
        push    %eax
        mov     16(%ebp), %eax
        push    %eax
        mov     12(%ebp), %eax
        push    %eax
/ Calls the entry point.
        mov     8(%ebp), %eax
        call    *%eax
        pop     %ebp
        ret

/////////////////////////////////////////////////////////////
/ void loadTaskRegister(unsigned short gdt_index);
/ Loads GDT index of TSS descriptor into Task Register.
_loadTaskRegister:
        push    %ebp
        mov     %esp, %ebp
        mov     8(%ebp), %eax
        ltr     %ax
        pop     %ebp
        ret

.bss
        .p2align 2
        .globl call32_esp
call32_esp:
        .space 4
