PDOSSUP TITLE 'P D O S S U P  ***  SUPPORT ROUTINE FOR PDOS'
***********************************************************************
*                                                                     *
*  THIS PROGRAM WRITTEN BY PAUL EDWARDS.                              *
*  RELEASED TO THE PUBLIC DOMAIN                                      *
*                                                                     *
***********************************************************************
***********************************************************************
*                                                                     *
*  PDOSSUP - assembler support routines for PDOS                      *
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
         MVC   SVCNPSW(8),NEWSVC
*
* Prepare CR6 for interrupts
         AIF   ('&SYS' NE 'S390').SIO24A
         LCTL  6,6,ALLIOINT CR6 needs to enable all interrupts
.SIO24A  ANOP
*
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
         AIF   ('&SYS' NE 'S390').NOT390A
         DS    0F
ALLIOINT DC    X'FF000000'
.NOT390A ANOP
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
NEWSVC   DC    X'040C0000'  machine check, EC, DAT on
         AIF   ('&SYS' EQ 'S370').MOD24B
         DC    A(X'80000000'+GOTSVC)  SVC handler
         AGO   .MOD31B
.MOD24B  ANOP
         DC    A(GOTSVC)    SVC handler
.MOD31B  ANOP
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
         STCM  R2,B'0111',LOADCCW+1   This requires BTL buffer
         L     R7,20(R1)    Bytes to read
         STH   R7,LOADCCW+6  Store in READ CCW
*
* Interrupt needs to point to CONT now. Again, I would hope for
* something more sophisticated in PDOS than this continual
* initialization.
*
         MVC   FLCINPSW(8),NEWIO
* R3 points to CCW chain
         LA    R3,SEEK
         ST    R3,FLCCAW    Store in CAW
*
*
         AIF   ('&SYS' EQ 'S390').SIO31B
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
         AIF   ('&SYS' EQ 'S390').SIO31H
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
         AIF   ('&SYS' NE 'S390').NOT390B
         DS    0F
IRB      DS    24F
ORB      DS    0F
         DC    F'0'
         DC    X'0000FF00'  Logical-Path Mask (enable all?)
         DC    A(SEEK)
         DC    5F'0'
.NOT390B ANOP
*
*
         DS    0D
SEEK     CCW   7,BBCCHH,X'40',6       40 = chain command
SEARCH   CCW   X'31',CCHHR,X'40',5    40 = chain command
         CCW   8,SEARCH,0,0
LOADCCW  CCW   6,0,X'20',32767        20 = ignore length issues
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
NEWIO    DC    X'040C0000'  machine check, EC, DAT on
         AIF   ('&SYS' EQ 'S370').MOD24
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
**********************************************************************
*                                                                    *
*  ADISP - dispatch a bit of code                                    *
*                                                                    *
**********************************************************************
         ENTRY ADISP
ADISP    DS    0H
         SAVE  (14,12),,ADISP
         LR    R12,R15
         USING ADISP,R12
         USING PSA,R0
*
         STM   R0,R15,FLCCRSAV        Save our OS registers
         LM    R0,R15,FLCGRSAV        Load application registers
         LPSW  SVCOPSW                App returns to old PSW
         DC    H'0'
ADISPRET DS    0H
         LA    R15,0
ADISPRT2 DS    0H
         RETURN (14,12),RC=(15)
         LTORG
*
*
*
**********************************************************************
*                                                                    *
*  GOTSVC - got an SVC interrupt                                     *
*                                                                    *
*  Need to go back through the dispatcher, which is waiting for this *
*                                                                    *
**********************************************************************
*
GOTSVC   DS    0H
         STM   R0,R15,FLCGRSAV        Save application registers
         LM    R0,R15,FLCCRSAV        Load OS registers
         B     ADISPRET
*
*
*
**********************************************************************
*                                                                    *
*  GOTRET - the application has simply returned with BR R14          *
*                                                                    *
*  Need to go back through the dispatcher. The easiest way to do     *
*  this is simply execute SVC 3, which the dispatcher has special    *
*  processing for.                                                   *
*                                                                    *
**********************************************************************
*
         ENTRY GOTRET
GOTRET   DS    0H
* force an SVC - it will take care of the rest
         SVC   3
         DC    H'0'                   PDOS should not return here
*
*
*
**********************************************************************
*                                                                    *
*  DREAD - DCB read routine (for when people do READ)                *
*                                                                    *
*  for now - go back via the same path as an SVC                     *
*                                                                    *
*  Relies on R12 being restored to its former glory, since we saved  *
*  it before running the application                                 *
*                                                                    *
**********************************************************************
*
         ENTRY DREAD
DREAD    DS    0H
         STM   R0,R15,FLCGRSAV        Save application registers
         ST    R14,SVCOPSW+4
         AIF   ('&SYS' EQ 'S390').MOD31G
         MVI   SVCOPSW+4,X'00'
.MOD31G  ANOP
         LM    R0,R15,FLCCRSAV        Load OS registers
*
* We need to return to 31-bit mode, which PDOS may be operating in.
         AIF   ('&SYS' EQ 'S370').MOD24G
         CALL  @@SETM31
.MOD24G  ANOP
         LA    R15,3
         B     ADISPRT2
*
*
*
**********************************************************************
*                                                                    *
*  DWRITE - DCB write routine (for when people do WRITE)             *
*                                                                    *
*  for now - go back via the same path as an SVC                     *
*                                                                    *
**********************************************************************
*
         ENTRY DWRITE
DWRITE   DS    0H
         STM   R0,R15,FLCGRSAV        Save application registers
         ST    R14,SVCOPSW+4
         AIF   ('&SYS' EQ 'S390').MOD31D
         MVI   SVCOPSW+4,X'00'
.MOD31D  ANOP
         LM    R0,R15,FLCCRSAV        Load OS registers
*
* We need to return to 31-bit mode, which PDOS may be operating in.
         AIF   ('&SYS' EQ 'S370').MOD24D
         CALL  @@SETM31
.MOD24D  ANOP
         LA    R15,2
         B     ADISPRT2
         DROP  ,
*
*
*
**********************************************************************
*                                                                    *
*  DCHECK - DCB check routine (for when people do CHECK)             *
*                                                                    *
*  for now, do nothing, since writes are executed synchronously      *
*                                                                    *
**********************************************************************
*
         ENTRY DCHECK
DCHECK   DS    0H
         BR    R14
*
*
*
*
**********************************************************************
*                                                                    *
*  DEXIT - DCB exit                                                  *
*                                                                    *
*  This is for when very annoying people have used a DCB exit which  *
*  needs to be called in the middle of doing an open or RDJFCB       *
*  (I wonder which one?)                                             *
*                                                                    *
*  They are expecting a DCB in R1                                    *
*                                                                    *
*  This routine is expecting the address of their exit as the first  *
*  parameter, and the address of the DCB as the second.              *
*                                                                    *
**********************************************************************
         ENTRY DEXIT
DEXIT    DS    0H
         SAVE  (14,12),,DEXIT
         LR    R12,R15
         USING DEXIT,R12
         USING PSA,R0
*
* Might need to switch save areas here
*
         L     R2,0(R1)               their exit
         L     R3,4(R1)               actual DCB for them
         AIF   ('&SYS' EQ 'S370').MOD24E
         CALL  @@SETM24
.MOD24E  ANOP
*
         LR    R15,R2
         LR    R1,R3
         BALR  R14,R15
*
         AIF   ('&SYS' EQ 'S370').MOD24F
         CALL  @@SETM31
.MOD24F  ANOP
*
DEXITRET DS    0H
         LA    R15,0
         RETURN (14,12),RC=(15)
         LTORG
*DEXITSAV DS    19F
*
*
*
**********************************************************************
*                                                                    *
*  LCREG0 - load control register 0                                  *
*                                                                    *
*  parameter 1 = value                                               *
*                                                                    *
**********************************************************************
         ENTRY LCREG0
LCREG0   DS    0H
         SAVE  (14,12),,LCREG0
         LR    R12,R15
         USING LCREG0,R12
*
         LCTL  0,0,0(R1)
*
         LA    R15,0
         RETURN (14,12),RC=(15)
         LTORG
**********************************************************************
*                                                                    *
*  LCREG1 - load control register 1                                  *
*                                                                    *
*  parameter 1 = value                                               *
*                                                                    *
**********************************************************************
         ENTRY LCREG1
LCREG1   DS    0H
         SAVE  (14,12),,LCREG1
         LR    R12,R15
         USING LCREG1,R12
*
         LCTL  1,1,0(R1)
*
         LA    R15,0
         RETURN (14,12),RC=(15)
         LTORG
**********************************************************************
*                                                                    *
*  LCREG13 - load control register 13                                *
*                                                                    *
*  parameter 1 = value                                               *
*                                                                    *
**********************************************************************
         ENTRY LCREG13
LCREG13  DS    0H
         SAVE  (14,12),,LCREG13
         LR    R12,R15
         USING LCREG13,R12
*
         LCTL  13,13,0(R1)
*
         LA    R15,0
         RETURN (14,12),RC=(15)
         LTORG
**********************************************************************
*                                                                    *
*  DATON - switch on DAT                                             *
*                                                                    *
**********************************************************************
         ENTRY DATON
DATON    DS    0H
         SAVE  (14,12),,DATON
         LR    R12,R15
         USING DATON,R12
*
         STOSM CURRMASK,X'04'
*
         LA    R15,0
         RETURN (14,12),RC=(15)
         LTORG
**********************************************************************
*                                                                    *
*  DATOFF - switch off DAT                                           *
*                                                                    *
**********************************************************************
         ENTRY DATOFF
DATOFF   DS    0H
         SAVE  (14,12),,DATOFF
         LR    R12,R15
         USING DATOFF,R12
*
         STNSM CURRMASK,X'FB'
*
         LA    R15,0
         RETURN (14,12),RC=(15)
         LTORG
CURRMASK DS    C
**********************************************************************
*                                                                    *
*  DSECTS                                                            *
*                                                                    *
**********************************************************************
         CVT   DSECT=YES
         IKJTCB
         IEZJSCB
         IHAPSA
         IHARB
         IHACDE
         IHASVC
         END
