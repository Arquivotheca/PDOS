**********************************************************************
*                                                                    *
*  THIS PROGRAM WRITTEN BY PAUL EDWARDS.                             *
*  RELEASED TO THE PUBLIC DOMAIN                                     *
*                                                                    *
**********************************************************************
**********************************************************************
*                                                                    *
*  VSESTART - STARTUP ROUTINES FOR VSE FOR USE WITH GCC.             *
*                                                                    *
*  Just make this call @@START. Don't worry about passing any        *
*  parameters, they will be ignored for now.                         *
*                                                                    *
**********************************************************************
         COPY  PDPTOP
.NOMODE ANOP
         PRINT GEN
* YREGS IS NOT AVAILABLE WITH IFOX
*         YREGS
R0       EQU   0
R1       EQU   1
R2       EQU   2
R3       EQU   3
R4       EQU   4
R5       EQU   5
R6       EQU   6
R7       EQU   7
R8       EQU   8
R9       EQU   9
R10      EQU   10
R11      EQU   11
R12      EQU   12
R13      EQU   13
R14      EQU   14
R15      EQU   15
SUBPOOL  EQU   0
@@VSESTR CSECT
         ENTRY @@CRT0
@@CRT0   EQU   *
*         ENTRY CEESTART
*CEESTART EQU   *
*         SAVE  (14,12),,@@CRT0
         LR    R8,R15            save R15 so that we can get the PARM
         BALR  R15,0
         USING *,R10
         LR    R10,R15
         LA    R10,0(R10)
*         USING @@CRT0,R10
*LOOP     LA    R5,1(R5)
*         B     LOOP
         LR    R11,R1            save R1 so we can get the PARM
         LR    R9,R13            save R13 so we can get flag byte
*         GETMAIN R,LV=STACKLEN,SP=SUBPOOL
         GETVIS LENGTH=STACKLEN
*LOOP     B     LOOP
         ST    R13,4(R1)
*         ST    R1,8(R13)
         LR    R13,R1
         USING STACK,R13
*DW* SAVE STACK POINTER FOR SETJMP/LONGJMP
*         EXTRN @@MANSTK
*         L     R3,=V(@@MANSTK)
*         ST    R13,0(R3)
*         L     R2,=A(STACKLEN)
*         ST    R2,4(R3)
*DW END OF MOD
*
         LA    R2,0
         ST    R2,DUMMYPTR       WHO KNOWS WHAT THIS IS USED FOR
         LA    R2,MAINSTK
         ST    R2,THEIRSTK       NEXT AVAILABLE SPOT IN STACK
         LA    R12,ANCHOR
         ST    R14,EXITADDR
         L     R3,=A(MAINLEN)
         AR    R2,R3
         ST    R2,12(R12)        TOP OF STACK POINTER
         LA    R2,0
         ST    R2,116(R12)       ADDR OF MEMORY ALLOCATION ROUTINE
*
* Now let's get the parameter list
*
         COMRG                   get address of common region in R1
         LR    R5,R1             use R5 to map common region
         USING COMREG,R5         address common region
         L     R2,SYSPAR         get access to SYSPARM
         LA    R2,0(R2)          clean the address, just in case
         ST    R2,ARGPTR         store SYSPARM
         LA    R2,0              default no VSE-style PARM
         CR    R11,R8            compare original R15 and original R1
         BE    CONTPARM          no difference = no VSE-style PARM
         LR    R2,R11            R11 has PARM, now R2 does too
CONTPARM DS    0H
         ST    R2,ARGPTRE        store VSE-style PARM
*
* Set R4 to true if we were called in 31-bit mode
*
         LA    R4,0
         AIF   ('&SYS' EQ 'S370').NOBSM
         BSM   R4,R0
.NOBSM   ANOP
         ST    R4,SAVER4
         LA    R2,PGMNAME
         ST    R2,PGMNPTR        store program name
*
* FOR GCC WE NEED TO BE ABLE TO RESTORE R13
         LA    R5,SAVEAREA
         ST    R5,SAVER13
*
         LA    R1,PARMLIST
*
         AIF   ('&SYS' NE 'S380').N380ST1
* If we were called in AMODE 31, don't bother setting mode now
         LTR   R4,R4
         BNZ   IN31
         CALL  @@SETM31
IN31     DS    0H
.N380ST1 ANOP
*
         CALL  @@START
         LR    R9,R15
*
         AIF   ('&SYS' NE 'S380').N380ST2
* If we were called in AMODE 31, don't switch back to 24-bit
         LTR   R4,R4
         BNZ   IN31B
         CALL  @@SETM24
IN31B    DS    0H
.N380ST2 ANOP
*
RETURNMS DS    0H
         LR    R1,R13
         L     R13,SAVEAREA+4
         LR    R14,R9
         FREEVIS LENGTH=STACKLEN
         LR    R15,R14
         EOJ
SAVER4   DS    F
SAVER13  DS    F
         LTORG
*         ENTRY CEESG003
*CEESG003 EQU   *
         DS    0H
         ENTRY @@EXITA
@@EXITA  EQU   *
* SWITCH BACK TO OUR OLD SAVE AREA
         LR    R10,R15
         USING @@EXITA,R10
         L     R9,0(R1)
         L     R13,=A(SAVER13)
         L     R13,0(R13)
*
         AIF   ('&SYS' NE 'S380').N380ST3
         L     R4,=A(SAVER4)
         L     R4,0(R4)
* If we were called in AMODE 31, don't switch back to 24-bit
         LTR   R4,R4
         BNZ   IN31C
         CALL  @@SETM24
IN31C    DS    0H
.N380ST3 ANOP
*
         LR    R1,R13
         L     R13,4(R13)
         LR    R14,R9
         FREEVIS LENGTH=STACKLEN
         LR    R15,R14
*         RETURN (14,12),RC=(15)
         EOJ
         LTORG
STACKLEN DC    A(STKLTMP)
*
STACK    DSECT
SAVEAREA DS    18F
DUMMYPTR DS    F
THEIRSTK DS    F
PARMLIST DS    0F
ARGPTR   DS    F
PGMNPTR  DS    F
ARGPTRE  DS    F
TYPE     DS    F
PGMNAME  DS    CL8
PGMNAMEN DS    C                 NUL BYTE FOR C
ANCHOR   DS    0F
EXITADDR DS    F
         DS    49F
MAINSTK  DS    32000F
MAINLEN  EQU   *-MAINSTK
STKLTMP  EQU   *-STACK
*         NUCON
         AIF   ('&SYS' NE 'S380').N380ST4
*         USERSAVE
.N380ST4 ANOP
COMREG   MAPCOMR
         END   @@CRT0
