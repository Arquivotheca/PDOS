SAPSTART TITLE 'S A P S T A R T  ***  STARTUP ROUTINE FOR C'
***********************************************************************
*                                                                     *
*  THIS PROGRAM WRITTEN BY PAUL EDWARDS.                              *
*  RELEASED TO THE PUBLIC DOMAIN                                      *
*                                                                     *
***********************************************************************
***********************************************************************
*                                                                     *
*  SAPSTART - startup routines for standalone programs                *
*  It is currently coded to work with GCC. To activate the C/370      *
*  version change the "&COMP" switch.                                 *
*                                                                     *
*  These routines are designed to work in conjunction with the        *
*  Hercules/380 dasdload, which will split a program into multiple    *
*  18452 blocks. It will attempt to read approximately 1 MB of these  *
*  blocks.                                                            *
*                                                                     *
***********************************************************************
*
         COPY  PDPTOP
*
         PRINT GEN
         YREGS
SUBPOOL  EQU   0
         CSECT
*
* This program will be loaded by the IPL sequence to location 0
* in memory. As such, we need to zero out the lower 512 bytes of
* memory which the hardware will use. Except for the first 8
* bytes, where we need to specify the new PSW.
*
         DS    0D
         DC    X'000C0000'  EC mode + Machine Check enabled
         DC    A(POSTIPL)   First bit of "normal" memory
*
* Memory to be cleared.
*
         DC    504X'00'
*
* Start of our own, somewhat normal, code. Registers are not
* defined at this point, so we need to create our own base
* register.
*
POSTIPL  DS    0H
         BALR  R12,0
         BCTR  R12,0
         BCTR  R12,0
         USING POSTIPL,R12
         USING PSA,R0
*
* At this point, since it is post-IPL, all further interrupts
* will occur to one of 4 locations (instead of location 0, the
* IPL newpsw). Although we are only expecting, and only need,
* the I/O interrupts, we set "dummy" values for the others in
* case something unexpected happens, to give us some visibility
* into the problem.
*
         MVC   FLCINPSW(8),NEWIO
         MVC   FLCMNPSW(8),WAITER1
         MVC   FLCSNPSW(8),WAITER2
         MVC   FLCPNPSW(8),WAITER3
* Save IPL address in R10
         SLR   R10,R10
         ICM   R10,B'0111',FLCIOAA
* R3 points to CCW chain
         LA    R3,SEEK
         ST    R3,FLCCAW    Store in CAW
*         CLRIO 0(R10)
         LA    R4,1         R4 = Number of blocks read so far
         L     R5,=F'18452' Current address
         LA    R6,0         R6 = head
         LA    R7,5         R7 = record
         SIO   0(R10)
         LPSW  WAITNOER     Wait for an interrupt
         LTORG
         DS    0D
WAITNOER DC    X'020E0000'  I/O, machine check, EC, wait
         DC    X'00000000'  no error
NEWIO    DC    X'020C0000'  I/O and machine check enabled + EC
         DC    A(STAGE2)
WAITER1  DC    X'020E0000'  I/O, machine check, EC, wait
         DC    X'00000111'  error 111
WAITER2  DC    X'020E0000'  I/O, machine check, EC, wait
         DC    X'00000222'  error 222
WAITER3  DC    X'020E0000'  I/O, machine check, EC, wait
         DC    X'00000333'  error 3
         DS    0D
SEEK     CCW   7,BBCCHH,X'40',6
SEARCH   CCW   X'31',CCHHR,X'40',5
         CCW   8,SEARCH,0,0
LOADCCW  CCW   6,TEXTADDR,X'20',32767
         DS    0H
BBCCHH   DC    X'000000000000'
         ORG   *-2
HH1      DS    CL2
CCHHR    DC    X'0000000005'
         ORG   *-3
HH2      DS    CL2
R        DS    C
*
TEXTADDR EQU   18452    This needs to be replaced
*
STAGE2   DS    0H
         A     R5,=F'18452'
         STCM  R5,B'0111',LOADCCW+1
         LA    R7,1(R7)
         C     R7,=F'4'
         BL    INRANGE
         LA    R7,1
         LA    R6,1(R6)
INRANGE  DS    0H
         STC   R7,R         
         STCM  R6,B'0011',HH1
         STCM  R6,B'0011',HH2
         LA    R4,1(R4)
         C     R4,=F'2'     Maximum blocks to read
         BH    STAGE3
         SIO   0(R10)       Read next block
         LPSW  WAITNOER
STAGE3   DS    0H
*         CLRIO 0(R10)
         LA    R4,0
*         L     R6,=X'E000000B'
         L     R6,=X'0000000B'
         LA    R4,=C'MSG * HELLO'
*         DIAG  4,6,0(8)
         DC    X'83460008'
         LA    R4,=C'MSG * 2 You'
         L     R6,=X'0000000B'
         DC    X'83460008'
LOOP     DS    0H
         LA    R5,7
         LA    R9,LOOP
         BR    R9
         DC    C'PDPCLIB!'
         LTORG
         DS    0H
         ENTRY @@CRT0
@@CRT0   EQU   *
         AIF ('&COMP' NE 'C370').NOCEES
         ENTRY CEESTART
CEESTART EQU   *
.NOCEES  ANOP
         SAVE  (14,12),,@@CRT0
         LR    R10,R15
         USING @@CRT0,R10
         LR    R11,R1
         GETMAIN RU,LV=STACKLEN,SP=SUBPOOL
         ST    R13,4(R1)
         ST    R1,8(R13)
         LR    R13,R1
         LR    R1,R11
         USING STACK,R13
*
         LA    R1,0(R1)          Clean up address (is this required?)
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
         LH    R2,JSCBTJID
         ST    R2,TYPE
         L     R2,0(R1)
         LA    R2,0(R2)
         ST    R2,ARGPTR
         LA    R2,PGMNAME
         ST    R2,PGMNPTR
*
* FOR GCC WE NEED TO BE ABLE TO RESTORE R13
         LA    R5,SAVEAREA
         ST    R5,SAVER13
*
         LA    R1,PARMLIST
*
         AIF   ('&SYS' NE 'S380').N380ST1
*
* Set R4 to true if we were called in 31-bit mode
*
         LA    R4,0
         BSM   R4,R0
         ST    R4,SAVER4
* If we were called in AMODE 31, don't bother setting mode now
         LTR   R4,R4
         BNZ   IN31
         CALL  @@SETM31
IN31     DS    0H
.N380ST1 ANOP
*
         CALL  @@START
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
         LR    R14,R15
         FREEMAIN RU,LV=STACKLEN,A=(R1),SP=SUBPOOL
         LR    R15,R14
         RETURN (14,12),RC=(15)
SAVER4   DS    F
SAVER13  DS    F
         LTORG
         DS    0H
*         ENTRY CEESG003
*CEESG003 EQU   *
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
         FREEMAIN RU,LV=STACKLEN,A=(R1),SP=SUBPOOL
         LR    R15,R14
         RETURN (14,12),RC=(15)
         LTORG
*
         CVT   DSECT=YES
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
ANCHOR   DS    0F
EXITADDR DS    F
         DS    49F
MAINSTK  DS    65536F
MAINLEN  EQU   *-MAINSTK
STACKLEN EQU   *-STACK
         END
