MVSSTART TITLE 'M V S S T A R T  ***  STARTUP ROUTINE FOR C'
***********************************************************************
*                                                                     *
*  THIS PROGRAM WRITTEN BY PAUL EDWARDS.                              *
*  RELEASED TO THE PUBLIC DOMAIN                                      *
*                                                                     *
***********************************************************************
***********************************************************************
*                                                                     *
*  MVSSTART - startup routines for MVS.                               *
*  It is currently coded to work with GCC. To activate the C/370      *
*  version change the "&COMP" switch.                                 *
*                                                                     *
***********************************************************************
*
         COPY  PDPTOP
*
         PRINT GEN
* YREGS was not part of the SYS1.MACLIB shipped with MVS 3.8j
* so may not be available, so do our own defines instead.
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
         CSECT
*
* Put an eyecatcher here to ensure program has been linked
* correctly.
         DC    C'PDPCLIB!'
*
         ENTRY @@CRT0
@@CRT0   DS    0H
         AIF ('&COMP' NE 'C370').NOCEES
         ENTRY CEESTART
CEESTART DS    0H
.NOCEES  ANOP
         SAVE  (14,12),,@@CRT0
         LR    R10,R15
         USING @@CRT0,R10
         LR    R11,R1
* Keep stack BTL so that the save area traceback works on MVS/380 2.0
         GETMAIN RU,LV=STACKLEN,SP=SUBPOOL,LOC=BELOW
         ST    R13,4(,R1)
         ST    R1,8(,R13)
         LR    R13,R1
         LR    R1,R11
         USING STACK,R13
*
         LA    R1,0(R1)          Clean up address (is this required?)
*
         LA    R2,0
         ST    R2,DUMMYPTR       We are not using a CRAB at this stage
*                                but GCC reserves this spot for a CRAB
         LA    R2,MAINSTK
         ST    R2,THEIRSTK       NEXT AVAILABLE SPOT IN STACK
*                                It seems that GCC and IBM C share this
*                                convention.
         LA    R12,ANCHOR        R12 is presumably reserved by IBM C
*                                for its answer to the SAS/C CRAB
*                                apparently called an "ANCHOR"
         ST    R14,EXITADDR      possibly used by IBM C for early exit
         L     R3,=A(MAINLEN)    get length of the main stack
         AR    R2,R3             pointer to top of the stack
         ST    R2,12(,R12)       IBM C needs to know stack end
*                                (GCC not currently checking overflow)
         LA    R2,0
         ST    R2,116(,R12)      ADDR OF MEMORY ALLOCATION ROUTINE
*                                (not used by GCC, but harmless)
         USING PSA,R0
         L     R2,PSATOLD
         USING TCB,R2
         L     R7,TCBRBP
         USING RBBASIC,R7
         LA    R8,0
         ICM   R8,B'0111',RBCDE1
         USING CDENTRY,R8
         MVC   PGMNAME,CDNAME
*
         L     R2,TCBJSCB
         USING IEZJSCB,R2
         SLR   R3,R3
         ICM   R3,B'0011',JSCBTJID
         ST    R3,TYPE           non-zero means TSO, 3rd parm to START
         L     R2,0(,R1)         get first program parameter
         LA    R2,0(,R2)         clean address
         ST    R2,ARGPTR         this will be first parm to START
         LA    R2,PGMNAME        find program name
         ST    R2,PGMNPTR        this will be second parm to START
*
* FOR GCC WE NEED TO BE ABLE TO RESTORE R13
         LA    R5,SAVEAREA
         ST    R5,SAVER13
*
         LA    R1,PARMLIST
*
         CALL  @@START           C code can now do everything else
*
RETURNMS DS    0H
         LR    R1,R13
         L     R13,SAVEAREA+4
         LR    R14,R15
         FREEMAIN RU,LV=STACKLEN,A=(R1),SP=SUBPOOL
         LR    R15,R14
         RETURN (14,12),RC=(15)
SAVER13  DS    F
         LTORG
         DS    0H
*         ENTRY CEESG003
*CEESG003 DS    0H
* This function enables GCC programs to do an
* early exit, ie callable from anywhere.
         ENTRY @@EXITA
@@EXITA  DS    0H
* SWITCH BACK TO OUR OLD SAVE AREA
         LR    R10,R15
         USING @@EXITA,R10
         L     R9,0(,R1)
         L     R13,=A(SAVER13)
         L     R13,0(,R13)
*
         LR    R1,R13
         L     R13,4(,R13)
         LR    R14,R9
         FREEMAIN RU,LV=STACKLEN,A=(R1),SP=SUBPOOL
         LR    R15,R14
         RETURN (14,12),RC=(15)
         LTORG
*
         IKJTCB
         IEZJSCB
         IHAPSA
         IHARB
         IHACDE
STACK    DSECT
SAVEAREA DS    18F
DUMMYPTR DS    F
THEIRSTK DS    F
PARMLIST DS    0F
ARGPTR   DS    F
PGMNPTR  DS    F
TYPE     DS    F
PGMNAME  DS    CL8
PGMNAMEN DS    C                 NUL BYTE FOR C
* This ANCHOR convention is only used by IBM C I think
* but is harmless to keep for GCC too
ANCHOR   DS    0F
EXITADDR DS    F
         DS    49F
MAINSTK  DS    65536F
MAINLEN  EQU   *-MAINSTK
STACKLEN EQU   *-STACK
         END
