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
*  Hercules/380 dasdload, which will create a disk with this program  *
*  on cylinder 0, head 1, but only read the first record into low     *
*  memory.  This startup code is required to read the remaining       *
*  blocks, and looks for approximately 1 MB of them.                  *
*                                                                     *
***********************************************************************
*
         COPY  PDPTOP
*
         PRINT GEN
         YREGS
***********************************************************************
*                                                                     *
*  Equates                                                            *
*                                                                     *
***********************************************************************
STACKLOC EQU   X'080000'    The stack starts here (0.5 MiB)
HEAPLOC  EQU   X'100000'    Where malloc etc come from (1 MiB)
CHUNKSZ  EQU   18452        The executable is split into blocks
MAXBLKS  EQU   40           Maximum number of blocks to read
CODESTRT EQU   1024         Start of our real code
ENTSTRT  EQU   2048         Create a predictable usable entry point
*
*
*
         CSECT
*
* This program will be loaded by the IPL sequence to location 0
* in memory. As such, we need to zero out the lower 512 bytes of
* memory which the hardware will use. Except for the first 8
* bytes, where we need to specify the new PSW.
*
ORIGIN   DS    0D
         DC    X'000C0000'  EC mode + Machine Check enabled
         AIF   ('&ZSYS' EQ 'S370').MOD24A
         DC    A(X'80000000'+POSTIPL)
         AGO   .MOD31A
.MOD24A  ANOP
         DC    A(POSTIPL)   First bit of "normal" memory
.MOD31A  ANOP
*
* Memory to be cleared.
*
         DC    (CODESTRT-*+ORIGIN)X'00'
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
         ICM   R10,B'1111',FLCIOA
* R3 points to CCW chain
         LA    R3,SEEK
         ST    R3,FLCCAW    Store in CAW
         LA    R4,1         R4 = Number of blocks read so far
         L     R5,=A(CHUNKSZ) Current address
         LA    R6,1         R6 = head
         LA    R7,2         R7 = record
         AIF   ('&ZSYS' EQ 'S390').SIO31A
         SIO   0(R10)
         AGO   .SIO24A
.SIO31A  ANOP
         LR    R1,R10       IPL subchannel needs to be in R1
         LCTL  6,6,ALLIOINT CR6 needs to enable all interrupts
         LA    R9,IRB
         TSCH  0(R9)
         LA    R10,ORB      R10 needs to point to ORB
         SSCH  0(R10)
.SIO24A  ANOP
         LPSW  WAITNOER     Wait for an I/O interrupt
         LTORG
*
*
*
         AIF   ('&ZSYS' NE 'S390').NOT390A
         DS    0F
ALLIOINT DC    X'FF000000'
IRB      DS    24F
ORB      DS    0F
         DC    F'0'
         DC    X'0000FF00'  Logical-Path Mask (enable all?)
         DC    A(SEEK)
         DC    5F'0'
.NOT390A ANOP
*
*
*
         DS    0D
WAITNOER DC    X'020E0000'  I/O, machine check, EC, wait
         DC    X'00000000'  no error
NEWIO    DC    X'000C0000'  machine check enabled + EC
         AIF   ('&ZSYS' EQ 'S370').MOD24B
         DC    A(X'80000000'+STAGE2)
         AGO   .MOD31B
.MOD24B  ANOP
         DC    A(STAGE2)
.MOD31B  ANOP
WAITER1  DC    X'000E0000'  machine check, EC, wait
         DC    X'00000111'  error 111
WAITER2  DC    X'000E0000'  machine check, EC, wait
         DC    X'00000222'  error 222
WAITER3  DC    X'000E0000'  machine check, EC, wait
         DC    X'00000333'  error 333
         DS    0D
SEEK     CCW   7,BBCCHH,X'40',6
SEARCH   CCW   X'31',CCHHR,X'40',5
         CCW   8,SEARCH,0,0
LOADCCW  CCW   6,CHUNKSZ,X'20',32767
         DS    0H
BBCCHH   DC    X'000000000001'
         ORG   *-2
HH1      DS    CL2
CCHHR    DC    X'0000000102'
         ORG   *-3
HH2      DS    CL2
R        DS    C
*
STAGE2   DS    0H
         A     R5,=A(CHUNKSZ)
         STCM  R5,B'0111',LOADCCW+1
         LA    R7,1(R7)
         C     R7,=F'4'     Don't read more than 3 blocks per track
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
         C     R4,=A(MAXBLKS)  R4=Maximum blocks to read
         BH    STAGE3
         AIF   ('&ZSYS' EQ 'S390').SIO31B
         SIO   0(R10)       Read next block
         AGO   .SIO24B
.SIO31B  ANOP
         TSCH  0(R9)
         SSCH  0(R10)
.SIO24B  ANOP
         LPSW  WAITNOER
STAGE3   DS    0H
* Go back to the original state, with I/O disabled, so that we
* don't get any more noise unless explicitly requested
         LPSW  ST4PSW
         DS    0D
ST4PSW   DC    X'000C0000'  EC mode + Machine Check enabled
         AIF   ('&ZSYS' EQ 'S370').MOD24C
         DC    A(X'80000000'+STAGE4)
         AGO   .MOD31C
.MOD24C  ANOP
         DC    A(STAGE4)
.MOD31C  ANOP
WAITSERR DC    X'000E0000'  EC mode + Machine Check enabled + wait
         DC    X'00000444'  Severe error
* At this point, we are in a "normal" post-IPL status,
* with our bootloader loaded, and interrupts disabled,
* and low memory should be considered to be in an
* "unknown" state. We will however pass a parameter
* block to the startup routine, with various bits of information
* for it to interpret.
STAGE4   DS    0H
* Since our program is less than 0.5 MB, set the stack at
* location 0.5 MB. Note that the other thing to worry about
* is the heap, which is set here, and returned in the sapsupa 
* GETM routine.
         L     R13,=A(STACKLOC)  Stack location
         LA    R2,0
         ST    R2,4(R13)         backchain to nowhere
         LR    R2,R13
         A     R2,=F'120'        Get past save area etc
         ST    R2,76(R13)        C needs to know where we're up to
*
         LA    R1,PRMPTR         MVS-style parm block (but to struct)
         L     R15,=V(@@CRT0)
         BALR  R14,R15
* If they're dumb enough to return, load an error wait state
         LPSW  WAITSERR
         LTORG
PRMPTR   DC    A(SAPBLK)
SAPBLK   DS    0F
SAPDUM   DC    F'0'
SAPLEN   DC    F'4'              Length of following parameters
HPLOC    DC    A(HEAPLOC)        Heap location
         DROP  ,
         DC    C'PDPCLIB!'
*
* This is the "main" entry point for standalone programs.
* Control can reach here via a number of methods. It may have
* been the result of booting from the card reader, with the
* destination being location 0. Or it may have been loaded
* by a stand-alone loader, and the destination is not location 0.
* However, in either case (or other cases, e.g. the startup
* code having to complete a load itself), the invoker of this
* code will have given a somewhat MVS-style parameter list.
* You can rely on R13 being a pointer to a save area, in
* fact, an actual stack. R1 will point to a fullword of 0,
* so that it looks like an empty parameter list, but following
* that, there will also be extra data, starting with a fullword
* which contains the length of that extra data block. R15 will
* be the entry point.
*
* The intention of all this is to allow any arbitrary
* stand alone program to be either loaded by a loader, anywhere
* in memory, or to be directly loadable into location 0. Multiple
* entry points, basically, but a common executable.
*
         DC    (ENTSTRT-*+ORIGIN)X'00'
         ORG   *-12
         DC    C'ZAPCONSL'
* Just before ordinary entry point, create a zappable variable
* to store a device number for a console.
         ENTRY @@CONSDN
@@CONSDN DC    F'0'
         DS    0H
         AIF ('&COMP' NE 'C370').NOCEES
         ENTRY CEESTART
CEESTART EQU   *
.NOCEES  ANOP
@@CRT0   PDPPRLG CINDEX=1,FRAME=120,BASER=12,ENTRY=YES
         B     FEN1
         LTORG
FEN1     EQU   *
         DROP  12
         BALR  12,0
         USING *,12
         LR    11,1
*
* Clean base register
         LA    R12,0(R12)
*
         USING STACK,R13
*
         LA    R2,0
         ST    R2,DUMMYPTR       WHO KNOWS WHAT THIS IS USED FOR
         LA    R2,MAINSTK
         ST    R2,THEIRSTK       NEXT AVAILABLE SPOT IN STACK
         LA    R7,ANCHOR
         ST    R14,EXITADDR
         L     R3,=A(MAINLEN)
         AR    R2,R3
         ST    R2,12(R7)         TOP OF STACK POINTER
         LA    R2,0
         ST    R2,116(R7)        ADDR OF MEMORY ALLOCATION ROUTINE
*         ST    R2,ARGPTR
*
         MVC   PGMNAME,=C'SAPLOAD '
*
         ST    R1,ARGPTR         pass the R1 directly on
         L     R1,0(R1)          It's a pointer to a structure
         L     R2,8(R1)          heap is available here
         ST    R2,@@HPLOC        heap location used by GETM
         LA    R2,PGMNAME
         ST    R2,PGMNPTR
*
* FOR GCC WE NEED TO BE ABLE TO RESTORE R13
         LA    R5,SAVEAREA
         ST    R5,SAVER13
*
         AIF   ('&ZSYS' NE 'S380').N380ST1
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
         AIF   ('&ZSYS' NE 'S380').N380ST2
* If we were called in AMODE 31, don't switch back to 24-bit
         LTR   R4,R4
         BNZ   IN31B
         CALL  @@SETM24
IN31B    DS    0H
.N380ST2 ANOP
*
RETURNMS DS    0H
         PDPEPIL
SAVER4   DC    F'0'
SAVER13  DC    F'0'
         LTORG
*
         ENTRY @@HPLOC
@@HPLOC  DS    A
         DROP  ,
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
         AIF   ('&ZSYS' NE 'S380').N380ST3
         L     R4,=A(SAVER4)
         L     R4,0(R4)
* If we were called in AMODE 31, don't switch back to 24-bit
         LTR   R4,R4
         BNZ   IN31C
         CALL  @@SETM24
IN31C    DS    0H
.N380ST3 ANOP
*
         PDPEPIL
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
