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
* At this stage we don't want any interrupts, but we need
* to set "dummy" values for all of them, to give us
* visibility into any problem.
*
         MVC   FLCINPSW(8),WAITER7
         MVC   FLCMNPSW(8),WAITER1
         MVC   FLCSNPSW(8),WAITER2
         MVC   FLCPNPSW(8),WAITER3
*
* Prepare CR6 for interrupts
         AIF   ('&ZSYS' NE 'S390').SIO24A
         LCTL  6,6,ALLIOINT CR6 needs to enable all interrupts
.SIO24A  ANOP
*
* Save IPL address in R10
* We should really obtain this from a parameter passed by
* sapstart.
*
         SLR   R10,R10
         ICM   R10,B'1111',FLCIOA
         LR    R15,R10
*
         RETURN (14,12),RC=(15)
         LTORG
*
*
*
         AIF   ('&ZSYS' NE 'S390').NOT390A
         DS    0F
ALLIOINT DC    X'FF000000'
.NOT390A ANOP
*
*
*
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
         L     R10,0(R1)    Device number
         L     R2,4(R1)     Cylinder
         STCM  R2,B'0011',CC1
         STCM  R2,B'0011',CC2
         L     R2,8(R1)     Head
         STCM  R2,B'0011',HH1
         STCM  R2,B'0011',HH2
         L     R2,12(R1)    Record
         STC   R2,R         
         L     R2,16(R1)    Buffer
* It is a requirement of using this routine that V=R. If it is
* ever required to support both V and R, then LRA could be used,
* and check for a 0 return, and if so, do a BNZ.
*         LRA   R2,0(R2)     Get real address
         L     R7,20(R1)    Bytes to read
         AIF   ('&ZSYS' EQ 'S390').CHN390B
         STCM  R2,B'0111',LOADCCW+1   This requires BTL buffer
         STH   R7,LOADCCW+6  Store in READ CCW
         AGO   .CHN390C
.CHN390B ANOP
         ST    R2,LOADCCW+4
         STH   R7,LOADCCW+2
.CHN390C ANOP
*
* Interrupt needs to point to CONT now. Again, I would hope for
* something more sophisticated in PDOS than this continual
* initialization.
*
         MVC   FLCINPSW(8),NEWIO
         STOSM FLCINPSW,X'00'  Work with DAT on or OFF
* R3 points to CCW chain
         LA    R3,SEEK
         ST    R3,FLCCAW    Store in CAW
*
*
         AIF   ('&ZSYS' EQ 'S390').SIO31B
         SIO   0(R10)
*         TIO   0(R10)
         AGO   .SIO24B
.SIO31B  ANOP
         LR    R1,R10       R1 needs to contain subchannel
         LA    R9,IRB
         TSCH  0(R9)        Clear pending interrupts
         LA    R10,ORB
         SSCH  0(R10)
.SIO24B  ANOP
*
*
         LPSW  WAITNOER     Wait for an interrupt
         DC    H'0'
CONT     DS    0H           Interrupt will automatically come here
         AIF   ('&ZSYS' EQ 'S390').SIO31H
         SH    R7,FLCCSW+6  Subtract residual count to get bytes read
         LR    R15,R7
* After a successful CCW chain, CSW should be pointing to end
         CLC   FLCCSW(4),=A(FINCHAIN)
         BE    ALLFINE
         AGO   .SIO24H
.SIO31H  ANOP
         TSCH  0(R9)
         SH    R7,10(R9)
         LR    R15,R7
         CLC   4(4,R9),=A(FINCHAIN)
         BE    ALLFINE
.SIO24H  ANOP
         L     R15,=F'-1'   error return
ALLFINE  DS    0H
         RETURN (14,12),RC=(15)
         LTORG
*
*
         AIF   ('&ZSYS' NE 'S390').NOT390B
         DS    0F
IRB      DS    24F
ORB      DS    0F
         DC    F'0'
         DC    X'0080FF00'  Logical-Path Mask (enable all?) + format-1
         DC    A(SEEK)
         DC    5F'0'
.NOT390B ANOP
*
*
         DS    0D
         AIF   ('&ZSYS' EQ 'S390').CHN390
SEEK     CCW   7,BBCCHH,X'40',6       40 = chain command
SEARCH   CCW   X'31',CCHHR,X'40',5    40 = chain command
         CCW   8,SEARCH,0,0
* X'E' = read key and data
LOADCCW  CCW   X'E',0,X'20',32767     20 = ignore length issues
         AGO   .CHN390F
.CHN390  ANOP
SEEK     CCW1  7,BBCCHH,X'40',6       40 = chain command
SEARCH   CCW1  X'31',CCHHR,X'40',5    40 = chain command
         CCW1  8,SEARCH,0,0
* X'E' = read key and data
LOADCCW  CCW1  X'E',0,X'20',32767     20 = ignore length issues
.CHN390F ANOP
FINCHAIN EQU   *
         DS    0H
BBCCHH   DC    X'000000000000'
         ORG   *-4
CC1      DS    CL2
HH1      DS    CL2
CCHHR    DC    X'0000000005'
         ORG   *-5
CC2      DS    CL2
HH2      DS    CL2
R        DS    C
         DS    0D
WAITNOER DC    X'060E0000'  I/O, machine check, EC, wait, DAT on
         DC    X'00000000'  no error
NEWIO    DC    X'000C0000'  machine check, EC, DAT off
         AIF   ('&ZSYS' EQ 'S370').MOD24
         DC    A(X'80000000'+CONT)  continuation after I/O request
         AGO   .MOD31
.MOD24   ANOP
         DC    A(CONT)      continuation after I/O request
.MOD31   ANOP
*
         DROP  ,
*
*
*
*
*
*
         CVT   DSECT=YES
         IKJTCB
         IEZJSCB
         IHAPSA
         IHARB
         IHACDE
         END
