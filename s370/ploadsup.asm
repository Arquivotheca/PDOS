PLOADSUP TITLE 'P L O A D S U P  ***  SUPPORT ROUTINE FOR PLOAD'
***********************************************************************
*                                                                     *
*  THIS PROGRAM WRITTEN BY PAUL EDWARDS.                              *
*  RELEASED TO THE PUBLIC DOMAIN                                      *
*                                                                     *
***********************************************************************
***********************************************************************
*                                                                     *
*  PLOADSUP - assembler support routines for PLOAD                    *
*  It is currently coded to work with GCC. To activate the C/370      *
*  version change the "&COMP" switch.                                 *
*                                                                     *
***********************************************************************
*
         COPY  PDPTOP
*
         PRINT GEN
         YREGS
SUBPOOL  EQU   0
         CSECT
**********************************************************************
*                                                                    *
*  INITSYS - initialize system                                       *
*                                                                    *
*  Note that at this point we can't assume what the status of the    *
*  interrupt vectors are, so we need to set them all to something    *
*  sensible ourselves. I/O will only be enabled when we want to do   *
*  an I/O.                                                           *
*                                                                    *
**********************************************************************
         ENTRY INITSYS
INITSYS  DS    0H
         SAVE  (14,12),,INITSYS
         LR    R12,R15
         USING INITSYS,R12
         USING PSA,R0
*
*         L     R6,0(R1)
*         L     R4,4(R1)
*
* At this stage we only want I/O interrupts, so that is all
* that will be enabled, but we set "dummy" values for the
* others in case something unexpected happens, to give us
* some visibility into the problem.
*
         MVC   FLCINPSW(8),WAITER7
         MVC   FLCMNPSW(8),WAITER1
         MVC   FLCSNPSW(8),WAITER2
         MVC   FLCPNPSW(8),WAITER3
* Save IPL address in R10
         SLR   R10,R10
         ICM   R10,B'0111',FLCIOAA
         LR    R15,R10
*
         RETURN (14,12),RC=(15)
         LTORG
         DS    0D
WAITER7  DC    X'000E0000'  machine check, EC, wait
         DC    X'00000777'  error 777
WAITER1  DC    X'000E0000'  machine check, EC, wait
         DC    X'00000111'  error 111
WAITER2  DC    X'000E0000'  machine check, EC, wait
         DC    X'00000222'  error 222
WAITER3  DC    X'000E0000'  machine check, EC, wait
         DC    X'00000333'  error 333
*
*
*
**********************************************************************
*                                                                    *
*  RDBLOCK - read a block from disk                                  *
*                                                                    *
*  parameter 1 = device                                              *
*  parameter 2 = cylinder                                            *
*  parameter 3 = head                                                *
*  parameter 4 = record                                              *
*  parameter 5 = buffer                                              *
*  parameter 6 = size of buffer                                      *
*                                                                    *
*  return = length of data read, or -1 on error                      *
*                                                                    *
**********************************************************************
         ENTRY RDBLOCK
RDBLOCK  DS    0H
         SAVE  (14,12),,RDBLOCK
         LR    R12,R15
         USING RDBLOCK,R12
         USING PSA,R0
*
* R3 points to CCW chain
         LA    R3,SEEK
         ST    R3,FLCCAW    Store in CAW
         LA    R4,1         R4 = Number of blocks read so far
         L     R5,=F'18452' Current address
         LA    R6,0         R6 = head
         LA    R7,5         R7 = record
         SIO   0(R10)
         LPSW  WAITNOER     Wait for an interrupt
CONT     DS    0H           Interrupt will automatically come here
         RETURN (14,12),RC=(15)
         LTORG
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
WAITNOER DC    X'020E0000'  I/O, machine check, EC, wait
         DC    X'00000000'  no error
NEWIO    DC    X'000C0000'  machine check, EC
         DC    A(CONT)      continuation after I/O request
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
* Ideally we want to read up until we have a short block, or
* an I/O error, but it's simpler to just force-read up to a
* set maximum.
         C     R4,=F'40'    Maximum blocks to read
         BH    STAGE3
         SIO   0(R10)       Read next block
         LPSW  WAITNOER
STAGE3   DS    0H
* Go back to the original state, with I/O disabled, so that we
* don't get any more noise unless explicitly requested
         LPSW  ST4PSW
         DS    0D
ST4PSW   DC    X'000C0000'  EC mode + Machine Check enabled
         DC    A(STAGE4)
WAITSERR DC    X'000E0000'  EC mode + Machine Check enabled + wait
         DC    X'00000444'  Severe error
* At this point, we are in a "normal" post-IPL status,
* with our bootloader loaded, and interrupts disabled,
* and low memory should be considered to be in an
* "unknown" state. We will however pass a parameter
* block to the startup routine, with various bits of information
* for it to interpret.
STAGE4   DS    0H
* Since our program is less than 1 MB, set the stack at
* location 1 MB. Note that the other thing to worry about
* is the heap, which is set in the sapsupa GETM routine
         L     R13,=F'1048576'   Stack location = 1 MB
         LA    R2,0
         ST    R2,4(R13)         backchain to nowhere
         LR    R2,R13
         A     R2,=F'120'
         ST    R2,76(R13)        Let them know top of memory
*
         LA    R1,SAPBLK         MVS-style parm block
         L     R15,=V(@@CRT0)
         BALR  R14,R15
* If they're dumb enough to return, load an error wait state
         LPSW  WAITSERR
         LTORG
SAPBLK   DS    0F
SAPDUM   DC    F'0'
SAPLEN   DC    F'4'              Length of following parameters
HPLOC    DC    F'1572864'        Heap location = 1.5 MB
         DROP  ,
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
