/ startup code for 32 bit version of pdos
/ written by Paul Edwards
/ released to the public domain

/ symbols defined outside of here that are accessed
        .globl _runaout_p
        .globl _pdosstrt

/ symbols defined here that are accessed from elsewhere
        .globl start
        .globl ___main
        .globl ___userparm

        .text

/////////////////////////////////////////////////////////////
/ void start(rawprot_parms *parmlist);

/ This is the entry point for the whole 32 bit PDOS executable.
/ It only takes one parameter, a pointer to some clumped parms.

start:
        push    %ebp
        mov     %esp, %ebp

/ Call runaout_p(parmlist) to set up anything it needs
        push    8(%ebp)
        call    _runaout_p        
        add     $4,%esp
        mov     %eax, ___userparm

/ Call main C entry point for PDOS.
        call    _pdosstrt

/ Return to PDOS loader
        pop     %ebp
        ret
