**********************************************************************
*                                                                    *
*  THIS PROGRAM WRITTEN BY PAUL EDWARDS.                             *
*  RELEASED TO THE PUBLIC DOMAIN                                     *
*                                                                    *
**********************************************************************
**********************************************************************
*                                                                    *
*  MVSSUPA - SUPPORT ROUTINES FOR PDPCLIB UNDER MVS                  *
*                                                                    *
**********************************************************************
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
*@@AOPEN  AMODE 31
*@@AOPEN  RMODE ANY
@@AOPEN  CSECT
         SAVE  (14,12),,@@AOPEN_&SYSDATE
         LR    R12,R15
         USING @@AOPEN,R12
         LR    R11,R1
         GETMAIN RU,LV=WORKLEN,SP=SUBPOOL
         ST    R13,4(R1)
         ST    R1,8(R13)
         LR    R13,R1
         LR    R1,R11
         USING WORKAREA,R13
*
         L     R3,0(R1)         R3 POINTS TO DDNAME
         L     R4,4(R1)         R4 POINTS TO MODE
         L     R5,8(R1)         R5 POINTS TO RECFM
         L     R8,12(R1)        R8 POINTS TO LRECL
*         GETMAIN RU,LV=ZDCBLEN,SP=SUBPOOL,LOC=BELOW
* CAN'T USE "BELOW" ON MVS 3.8
         GETMAIN RU,LV=ZDCBLEN,SP=SUBPOOL
         LR    R2,R1
* THIS LINE IS FOR GCC         
         LR    R6,R4
* THIS LINE IS FOR C/370         
*         L     R6,0(R4)
         LTR   R6,R6
         BNZ   WRITING
* READING
         USING ZDCBAREA,R2
         MVC   ZDCBAREA(INDCBLN),INDCB
         MVC   EOFR24(EOFRLEN),ENDFILE
         LA    R4,EOFR24
         USING IHADCB,R2
         MVC   DCBDDNAM,0(R3)
         MVC   OPENMB,OPENMAC
         STCM  R4,B'0111',DCBEODA
*         OPEN  ((R2),INPUT),MF=(E,OPENMB),MODE=31
* CAN'T USE MODE=31 ON MVS 3.8
         OPEN  ((R2),INPUT),MF=(E,OPENMB)
         B     DONEOPEN
WRITING  DS    0H
         USING ZDCBAREA,R2
         MVC   ZDCBAREA(OUTDCBLN),OUTDCB
         USING IHADCB,R2
         MVC   DCBDDNAM,0(R3)
         MVC   WOPENMB,WOPENMAC
*         OPEN  ((R2),OUTPUT),MF=(E,WOPENMB),MODE=31
* CAN'T USE MODE=31 ON MVS 3.8
         OPEN  ((R2),OUTPUT),MF=(E,WOPENMB)
DONEOPEN DS    0H
         SR    R6,R6
         LH    R6,DCBLRECL
         ST    R6,0(R8)
         SR    R6,R6
         IC    R6,DCBRECFM
         LA    R7,DCBRECF
         NR    R6,R7
         BZ    VARIABLE
         L     R6,=F'0'
         B     DONESET
VARIABLE DS    0H
         L     R6,=F'1'
DONESET  DS    0H
         ST    R6,0(R5)
         LR    R15,R2
         B     RETURN
*
* THIS IS NOT EXECUTED DIRECTLY, BUT COPIED INTO 24-BIT STORAGE
ENDFILE  LA    R6,1
         BR    R14
EOFRLEN  EQU   *-ENDFILE
*
RETURN   DS    0H
         LR    R1,R13
         L     R13,SAVEAREA+4
         LR    R14,R15
         FREEMAIN RU,LV=WORKLEN,A=(R1),SP=SUBPOOL
         LR    R15,R14
         RETURN (14,12),RC=(15)
         LTORG
* OPENMAC  OPEN  (,INPUT),MF=L,MODE=31
* CAN'T USE MODE=31 ON MVS 3.8
OPENMAC  OPEN  (,INPUT),MF=L
OPENMLN  EQU   *-OPENMAC
* WOPENMAC OPEN  (,OUTPUT),MF=L,MODE=31
* CAN'T USE MODE=31 ON MVS 3.8
WOPENMAC OPEN  (,OUTPUT),MF=L
WOPENMLN EQU   *-WOPENMAC
INDCB    DCB   MACRF=GL,DSORG=PS,EODAD=ENDFILE
INDCBLN  EQU   *-INDCB
OUTDCB   DCB   MACRF=PL,DSORG=PS
OUTDCBLN EQU   *-OUTDCB
*
*
*
*@@AREAD  AMODE 31
*@@AREAD  RMODE ANY
@@AREAD  CSECT
         SAVE  (14,12),,@@AREAD_&SYSDATE
         LR    R12,R15
         USING @@AREAD,R12
         LR    R11,R1
         GETMAIN RU,LV=WORKLEN,SP=SUBPOOL
         ST    R13,4(R1)
         ST    R1,8(R13)
         LR    R13,R1
         LR    R1,R11
         USING WORKAREA,R13
*
         L     R2,0(R1)         R2 CONTAINS HANDLE
         L     R3,4(R1)         R3 POINTS TO BUF POINTER
         LA    R6,0
         GET   (R2)
         ST    R1,0(R3)
         LR    R15,R6
         B     RETURN2
*
RETURN2  DS    0H
         LR    R1,R13
         L     R13,SAVEAREA+4
         LR    R14,R15
         FREEMAIN RU,LV=WORKLEN,A=(R1),SP=SUBPOOL
         LR    R15,R14
         RETURN (14,12),RC=(15)
*
*
*
*@@AWRITE AMODE 31
*@@AWRITE RMODE ANY
@@AWRITE CSECT
         SAVE  (14,12),,@@AWRITE_&SYSDATE
         LR    R12,R15
         USING @@AWRITE,R12
         LR    R11,R1
         GETMAIN RU,LV=WORKLEN,SP=SUBPOOL
         ST    R13,4(R1)
         ST    R1,8(R13)
         LR    R13,R1
         LR    R1,R11
         USING WORKAREA,R13
*
         L     R2,0(R1)         R2 CONTAINS HANDLE
         L     R3,4(R1)         R3 POINTS TO BUF POINTER
         PUT   (R2)
         ST    R1,0(R3)
         LA    R15,0
         B     RETURNWR
*
RETURNWR DS    0H
         LR    R1,R13
         L     R13,SAVEAREA+4
         LR    R14,R15
         FREEMAIN RU,LV=WORKLEN,A=(R1),SP=SUBPOOL
         LR    R15,R14
         RETURN (14,12),RC=(15)
*
*
*
*@@ACLOSE AMODE 31
*@@ACLOSE RMODE ANY
@@ACLOSE CSECT
         SAVE  (14,12),,@@ACLOSE_&SYSDATE
         LR    R12,R15
         USING @@ACLOSE,R12
         LR    R11,R1
         GETMAIN RU,LV=WORKLEN,SP=SUBPOOL
         ST    R13,4(R1)
         ST    R1,8(R13)
         LR    R13,R1
         LR    R1,R11
         USING WORKAREA,R13
*
         L     R2,0(R1)         R2 CONTAINS HANDLE
         N     R2,=X'7FFFFFFF'
         MVC   CLOSEMB,CLOSEMAC
*         CLOSE ((R2)),MF=(E,CLOSEMB),MODE=31
* CAN'T USE MODE=31 WITH MVS 3.8
         CLOSE ((R2)),MF=(E,CLOSEMB)
         FREEMAIN RU,LV=ZDCBLEN,A=(R2),SP=SUBPOOL
         LA    R15,0
         B     RETURN3
*
RETURN3  DS    0H
         LR    R1,R13
         L     R13,SAVEAREA+4
         LR    R14,R15
         FREEMAIN RU,LV=WORKLEN,A=(R1),SP=SUBPOOL
         LR    R15,R14
         RETURN (14,12),RC=(15)
         LTORG
* CLOSEMAC CLOSE (),MF=L,MODE=31
* CAN'T USE MODE=31 WITH MVS 3.8
CLOSEMAC CLOSE (),MF=L
CLOSEMLN EQU   *-CLOSEMAC
*
*
*
**********************************************************************
*                                                                    *
*  GETM - GET MEMORY                                                 *
*                                                                    *
**********************************************************************
*@@GETM   AMODE 31
*@@GETM   RMODE ANY
@@GETM   CSECT
         SAVE  (14,12),,@@GETM_&SYSDATE
         LR    R12,R15
         USING @@GETM,R12
*
         L     R2,0(R1)
* THIS LINE IS FOR GCC         
         LR    R3,R2
* THIS LINE IS FOR C/370
*         L     R3,0(R2)
         A     R3,=F'16'
         GETMAIN RU,LV=(R3),SP=SUBPOOL
         ST    R3,0(R1)
         A     R1,=F'16'
         LR    R15,R1
*
RETURNGM DS    0H
         RETURN (14,12),RC=(15)
         LTORG
*
*
*
**********************************************************************
*                                                                    *
*  FREEM - FREE MEMORY                                               *
*                                                                    *
**********************************************************************
*@@FREEM  AMODE 31
*@@FREEM  RMODE ANY
@@FREEM  CSECT
         SAVE  (14,12),,@@FREEM_&SYSDATE
         LR    R12,R15
         USING @@FREEM,R12
*
         L     R2,0(R1)
         S     R2,=F'16'
         L     R3,0(R2)
         FREEMAIN RU,LV=(R3),A=(R2),SP=SUBPOOL
*
RETURNFM DS    0H
         RETURN (14,12),RC=(15)
         LTORG
*
*
*
**********************************************************************
*                                                                    *
*  GETCLCK - GET THE VALUE OF THE MVS CLOCK TIMER AND MOVE IT TO AN  *
*  8-BYTE FIELD.  THIS 8-BYTE FIELD DOES NOT NEED TO BE ALIGNED IN   *
*  ANY PARTICULAR WAY.                                               *
*                                                                    *
*  E.G. CALL 'GETCLCK' USING WS-CLOCK1                               *
*                                                                    *
*  THIS FUNCTION ALSO RETURNS THE NUMBER OF SECONDS SINCE 1970-01-01 *
*  BY USING SOME EMPERICALLY-DERIVED MAGIC NUMBERS                   *
*                                                                    *
**********************************************************************
*@@GETCLK AMODE 31
*@@GETCLK RMODE ANY
@@GETCLK CSECT
         SAVE  (14,12),,@@GETCLK_&SYSDATE
         LR    R12,R15
         USING @@GETCLK,R12
*
         L     R2,0(R1)
         STCK  0(R2)
         L     R4,0(R2)
         L     R5,4(R2)
         SRDL  R4,12
         SL    R4,=X'0007D910'
         D     R4,=F'1000000'
         SL    R5,=F'1220'
         LR    R15,R5
*
RETURNGC DS    0H
         RETURN (14,12),RC=(15)
         LTORG
WORKAREA DSECT
SAVEAREA DS    18F
         DS    0F
CLOSEMB  DS    CL(CLOSEMLN)
         DS    0F
OPENMB   DS    CL(OPENMLN)
         DS    0F
WOPENMB  DS    CL(WOPENMLN)
WORKLEN  EQU   *-WORKAREA
ZDCBAREA DSECT
         DS    CL(INDCBLN)
         DS    CL(OUTDCBLN)
         DS    0H
EOFR24   DS    CL(EOFRLEN)
ZDCBLEN  EQU   *-ZDCBAREA
         DCBD  DSORG=PS
         END
