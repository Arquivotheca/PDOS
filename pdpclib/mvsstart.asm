**********************************************************************
*                                                                    *
*  THIS PROGRAM WRITTEN BY PAUL EDWARDS.                             *
*  RELEASED TO THE PUBLIC DOMAIN                                     *
*                                                                    *
**********************************************************************
**********************************************************************
*                                                                    *
*  MVSSTART - STARTUP ROUTINES FOR MVS FOR USE WITH C/370.           *
*                                                                    *
*  NOTE THAT THE AMODE/RMODE AND CSVQUERY HAVE BEEN COMMENTED        *
*  OUT IN ORDER TO BE COMPATIBLE WITH IFOX (OS/360).  ON OS/370      *
*  AND ABOVE THE AMODE/RMODE CAN BE GIVEN AT ASSEMBLY AND LINK       *
*  TIME INSTEAD.  WITHOUT CSVQUERY, WHEN RUN UNDER TSO, THE          *
*  PROGRAM NAME WILL BE IKJEFT01 INSTEAD OF THE NORMAL NAME, BUT     *
*  THAT WILL NOT AFFECT PROGRAM FUNCTIONALITY.                       *
*                                                                    *
**********************************************************************
*@@MVSTRT AMODE 31
*@@MVSTRT RMODE ANY
         ENTRY @@MVSTRT
@@MVSTRT CSECT
         PRINT NOGEN
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
         SAVE  (14,12),,@@MVSTRT_&SYSDATE
         LR    R10,R15
         USING @@MVSTRT,R10
         LR    R11,R1
         GETMAIN RU,LV=STACKLEN,SP=SUBPOOL
         ST    R13,4(R1)
         ST    R1,8(R13)
         LR    R13,R1
         LR    R1,R11
         USING STACK,R13
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
         ST    R2,ARGPTR
*
         L     R2,CVTPTR
         USING CVT,R2
         L     R2,CVTTCBP
         L     R2,4(R2)
         USING TCB,R2
         L     R2,TCBJSCB
         USING IEZJSCB,R2
         MVC   PGMNAME,JSCBPGMN
*
         LH    R2,JSCBTJID
         ST    R2,TYPE
         L     R2,0(R1)
         ST    R2,ARGPTR
         LA    R2,PGMNAME
         ST    R2,PGMNPTR
*
*         LA    R2,=V(@@MVSTRT)
*         CSVQUERY INADDR=(R2),OUTMJNM=PGMNAME,MF=(E,CSVQC)
*
         LA    R1,PARMLIST
         CALL  @@START
         B     RETURN
*
RETURN   DS    0H
         LR    R1,R13
         L     R13,SAVEAREA+4
         LR    R14,R15
         FREEMAIN RU,LV=STACKLEN,A=(R1),SP=SUBPOOL
         LR    R15,R14
         RETURN (14,12),RC=(15)
         LTORG
         CVT   DSECT=YES
         IKJTCB
         IEZJSCB
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
ANCHOR   DS    0F
EXITADDR DS    F
         DS    49F
*         CSVQUERY MF=(L,CSVQC)
MAINSTK  DS    4000F
MAINLEN  EQU   *-MAINSTK
STACKLEN EQU   *-STACK
*CEESTART AMODE 31
*CEESTART RMODE ANY
CEESTART CSECT
*CEESG003 AMODE 31
*CEESG003 RMODE ANY
CEESG003 CSECT
*@CRT0    AMODE 31
*@CRT0    RMODE ANY
         ENTRY @CRT0
@CRT0    CSECT
*@@EXITA  AMODE 31
*@@EXITA  RMODE ANY
         ENTRY @@EXITA
@@EXITA  CSECT
         L     R14,0(R12)
         L     R15,0(R1)
         BR    R14
         END
