***********************************************************************
*                                                                     *
*  This program written by Paul Edwards.                              *
*  Released to the public domain                                      *
*                                                                     *
***********************************************************************
***********************************************************************
*                                                                     *
*  amistart - startup code for AmigaOS.                               *
*                                                                     *
*  This uses the Motorola syntax                                      *
*                                                                     *
*  It puts the standard AmigaOS registers containing the command      *
*  length (d0) and command buffer (a0) as well as the AmigaPDOS       *
*  extension register (only visible if running AmigaPDOS) a6,         *
*  containing an alternative SysBase to use (only if d0 is greater    *
*  than or equal to 2 GiB, and in which case, 2 GiB should be         *
*  subtracted from d0 before use) on the stack.                       *
*                                                                     *
*  All this manipulation is left for the C startup code to handle.    *
*                                                                     *
***********************************************************************
*
        section "CODE",code
        xref ___start
        xdef ___amistart
        xdef ___exita

___amistart:
        movem.l d0/a0/a6,-(sp)
        jsr ___start
        rts

* This function receives a return code as a parameter. The stack
* then needs to be restored and the parameter placed in register d0
* prior to return to the OS.

___exita:
        rts
