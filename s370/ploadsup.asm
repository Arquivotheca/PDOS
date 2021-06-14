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
*
*
*
         AIF ('&ZSYS' EQ 'S370').AMB24A
AMBIT    EQU X'80000000'
         AGO .AMB24B
.AMB24A  ANOP
AMBIT    EQU X'00000000'
.AMB24B  ANOP
*
*
*
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
         DC    A(AMBIT+X'00000777')  error 777
WAITER1  DC    X'000E0000'  machine check, EC, wait
         DC    A(AMBIT+X'00000111')  error 111
WAITER2  DC    X'000E0000'  machine check, EC, wait
         DC    A(AMBIT+X'00000222')  error 222
WAITER3  DC    X'000E0000'  machine check, EC, wait
         DC    A(AMBIT+X'00000333')  error 333
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
