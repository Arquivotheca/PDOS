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
*  IT IS CURRENTLY CODED FOR GCC, BUT C/370 FUNCTIONALITY IS         *
*  STILL THERE, IT'S JUST COMMENTED OUT. I DON'T KNOW HOW TO DO      *
*  CONDITIONAL COMPILATION OF THAT.                                  *
*                                                                    *
**********************************************************************
         AIF ('&SYSPARM' EQ 'IFOX00').NOMODE
* BECAUSE OF THE "LOC=ABOVE", WE NEED TO FORCE 31
* SEARCH FOR "LOC=RES" TO FIND OUT HOW TO FIX
         AMODE 31
* SEARCH FOR "LOC=RES" TO FIND OUT WHY THIS IS BEING
* HELD BACK AT RMODE 24
         RMODE 24
.NOMODE ANOP
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
         CSECT
         ENTRY @@AOPEN
@@AOPEN  EQU   *
         SAVE  (14,12),,@@AOPEN
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
         L     R9,16(R1)        R9 POINTS TO MEMBER NAME (OF PDS)
         AIF   ('&SYSPARM' NE 'IFOX00').BELOW
* CAN'T USE "BELOW" ON MVS 3.8
         GETMAIN RU,LV=ZDCBLEN,SP=SUBPOOL
         AGO   .CHKBLWE
.BELOW   ANOP
         GETMAIN RU,LV=ZDCBLEN,SP=SUBPOOL,LOC=BELOW
.CHKBLWE ANOP
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
         LA    R10,JFCB
* EXIT TYPE 07 + 80 (END OF LIST INDICATOR)
         ICM   R10,B'1000',=X'87'
         ST    R10,JFCBPTR
         LA    R10,JFCBPTR
         LA    R4,EOFR24
         USING IHADCB,R2
         STCM  R4,B'0111',DCBEODA
         STCM  R10,B'0111',DCBEXLSA
         MVC   DCBDDNAM,0(R3)
         MVC   OPENMB,OPENMAC
* NOTE THAT THIS IS CURRENTLY NOT REENTRANT AND SHOULD BE MADE SO
         RDJFCB ((R2),INPUT)
         LTR   R9,R9
         BZ    NOMEM
         USING ZDCBAREA,R2
         MVC   JFCBELNM,0(R9)
         OI    JFCBIND1,JFCPDS
NOMEM    DS    0H
*         OPEN  ((R2),INPUT),MF=(E,OPENMB),MODE=31,TYPE=J
* CAN'T USE MODE=31 ON MVS 3.8, OR WITH TYPE=J
         OPEN  ((R2),INPUT),MF=(E,OPENMB),TYPE=J
         B     DONEOPEN
WRITING  DS    0H
         USING ZDCBAREA,R2
         MVC   ZDCBAREA(OUTDCBLN),OUTDCB
         LA    R10,JFCB
* EXIT TYPE 07 + 80 (END OF LIST INDICATOR)
         ICM   R10,B'1000',=X'87'
         ST    R10,JFCBPTR
         LA    R10,JFCBPTR
         USING IHADCB,R2
         STCM  R10,B'0111',DCBEXLSA
         MVC   DCBDDNAM,0(R3)
         MVC   WOPENMB,WOPENMAC
* NOTE THAT THIS IS CURRENTLY NOT REENTRANT AND SHOULD BE MADE SO
         RDJFCB ((R2),OUTPUT)
         LTR   R9,R9
         BZ    WNOMEM
         USING ZDCBAREA,R2
         MVC   JFCBELNM,0(R9)
         OI    JFCBIND1,JFCPDS
WNOMEM   DS    0H
*         OPEN  ((R2),OUTPUT),MF=(E,WOPENMB),MODE=31,TYPE=J
* CAN'T USE MODE=31 ON MVS 3.8, OR WITH TYPE=J
         OPEN  ((R2),OUTPUT),MF=(E,WOPENMB),TYPE=J
DONEOPEN DS    0H
         USING IHADCB,R2
         SR    R6,R6
         LH    R6,DCBLRECL
         ST    R6,0(R8)
         TM    DCBRECFM,DCBRECF
         BNO   VARIABLE
         L     R6,=F'0'
         B     DONESET
VARIABLE DS    0H
         L     R6,=F'1'
DONESET  DS    0H
         ST    R6,0(R5)
         LR    R15,R2
         B     RETURNOP
*
* THIS IS NOT EXECUTED DIRECTLY, BUT COPIED INTO 24-BIT STORAGE
ENDFILE  LA    R6,1
         BR    R14
EOFRLEN  EQU   *-ENDFILE
*
RETURNOP DS    0H
         LR    R1,R13
         L     R13,SAVEAREA+4
         LR    R14,R15
         FREEMAIN RU,LV=WORKLEN,A=(R1),SP=SUBPOOL
         LR    R15,R14
         RETURN (14,12),RC=(15)
         LTORG
* OPENMAC  OPEN  (,INPUT),MF=L,MODE=31
* CAN'T USE MODE=31 ON MVS 3.8
OPENMAC  OPEN  (,INPUT),MF=L,TYPE=J
OPENMLN  EQU   *-OPENMAC
* WOPENMAC OPEN  (,OUTPUT),MF=L,MODE=31
* CAN'T USE MODE=31 ON MVS 3.8
WOPENMAC OPEN  (,OUTPUT),MF=L
WOPENMLN EQU   *-WOPENMAC
*INDCB    DCB   MACRF=GL,DSORG=PS,EODAD=ENDFILE,EXLST=JPTR
* LEAVE OUT EODAD AND EXLST, FILLED IN LATER
INDCB    DCB   MACRF=GL,DSORG=PS
INDCBLN  EQU   *-INDCB
JPTR     DS    F
OUTDCB   DCB   MACRF=PL,DSORG=PS
OUTDCBLN EQU   *-OUTDCB
*
*
*
         ENTRY @@AREAD
@@AREAD  EQU   *
         SAVE  (14,12),,@@AREAD
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
*
RETURNAR DS    0H
         LR    R1,R13
         L     R13,SAVEAREA+4
         LR    R14,R15
         FREEMAIN RU,LV=WORKLEN,A=(R1),SP=SUBPOOL
         LR    R15,R14
         RETURN (14,12),RC=(15)
*
*
*
         ENTRY @@AWRITE
@@AWRITE EQU   *
         SAVE  (14,12),,@@AWRITE
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
*
RETURNAW DS    0H
         LR    R1,R13
         L     R13,SAVEAREA+4
         LR    R14,R15
         FREEMAIN RU,LV=WORKLEN,A=(R1),SP=SUBPOOL
         LR    R15,R14
         RETURN (14,12),RC=(15)
*
*
*
         ENTRY @@ACLOSE
@@ACLOSE EQU   *
         SAVE  (14,12),,@@ACLOSE
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
         MVC   CLOSEMB,CLOSEMAC
*         CLOSE ((R2)),MF=(E,CLOSEMB),MODE=31
* CAN'T USE MODE=31 WITH MVS 3.8
         CLOSE ((R2)),MF=(E,CLOSEMB)
         FREEPOOL ((R2))
         FREEMAIN RU,LV=ZDCBLEN,A=(R2),SP=SUBPOOL
         LA    R15,0
*
RETURNAC DS    0H
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
         ENTRY @@GETM
@@GETM   EQU   *
         SAVE  (14,12),,@@GETM
         LR    R12,R15
         USING @@GETM,R12
*
         L     R2,0(R1)
* THIS LINE IS FOR GCC
         LR    R3,R2
* THIS LINE IS FOR C/370
*         L     R3,0(R2)
         A     R3,=F'16'

*
* THIS SHOULD NOT BE NECESSARY. THE DEFAULT OF LOC=RES
* SHOULD BE SUFFICIENT. HOWEVER, CURRENTLY THERE IS AN
* UNKNOWN PROBLEM, PROBABLY WITH RDJFCB, WHICH PREVENTS
* EXECUTABLES FROM RESIDING ABOVE THE LINE, HENCE THIS
* HACK TO ALLOCATE MOST STORAGE ABOVE THE LINE
*                  
         AIF   ('&SYSPARM' NE 'IFOX00').ANYCHKY
* CAN'T USE "ANY" ON MVS 3.8
         GETMAIN RU,LV=(R3),SP=SUBPOOL
         AGO   .ANYCHKE
.ANYCHKY ANOP
         GETMAIN RU,LV=(R3),SP=SUBPOOL,LOC=ANY
.ANYCHKE ANOP
*
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
         ENTRY @@FREEM
@@FREEM  EQU   *
         SAVE  (14,12),,@@FREEM
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
         ENTRY @@GETCLK
@@GETCLK EQU   *
         SAVE  (14,12),,@@GETCLK
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
*
*
*
**********************************************************************
*                                                                    *
*  SAVER - SAVE REGISTERS AND PSW INTO ENV_BUF                       *
*                                                                    *
**********************************************************************
         ENTRY @@SAVER
@@SAVER EQU   *
*
         SAVE  (14,12),,SETJMP     * SAVE REGS AS NORMAL
         LR    R12,R15
         USING @@SAVER,12
         L     R1,0(R1)            * ADDRESS OF ENV TO R1
         L     R2,@@MANSTK         * R2 POINTS TO START OF STACK
         L     R3,@@MANSTK+4       * R3 HAS LENGTH OF STACK
         LR    R5,R3               * AND R5
         LR    R9,R1               * R9 NOW CONTAINS ADDRESS OF ENV
         GETMAIN R,LV=(R3),SP=SUBPOOL    * GET A SAVE AREA
         ST    R1,0(R9)            * SAVE IT IN FIRST WORK OF ENV
         ST    R5,4(R9)            * SAVE LENGTH IN SECOND WORD OF ENV
         ST    R2,8(R9)            * NOTE WHERE WE GOT IT FROM
         ST    R13,12(R9)          * AND R13
         LR    R4,R1               * AND R4
         MVCL  R4,R2               * COPY SETJMP'S SAVE AREA TO ENV
*        STM   R0,R15,0(R1)               SAVE REGISTERS
*        BALR  R15,0                     GET PSW INTO R15
*        ST    R15,64(R1)                 SAVE PSW
*
RETURNSR DS    0H
         SR    R15,R15              * CLEAR RETURN CODE
         RETURN (14,12),RC=(15)
         ENTRY   @@MANSTK
@@MANSTK DS    2F
         LTORG
*
*
*
**********************************************************************
*                                                                    *
*  LOADR - LOAD REGISTERS AND PSW FROM ENV_BUF                       *
*                                                                    *
**********************************************************************
         ENTRY @@LOADR
@@LOADR EQU   *
*
         BALR  R12,R0
         USING *,12
         L     R1,0(R1)           * R1 POINTS TO ENV
         L     R2,8(R1)           * R2 POINTS TO STACK
         L     R3,4(R1)           * R3 HAS HOW LONG
         LR    R5,R3              * AS DOES R5
         L     R6,24(R1)          * R6 HAS RETURN CODE
         L     R4,0(R1)           * OUR SAVE AREA
         L     R13,12(R1)         * GET OLD STACK POINTER
         MVCL  R2,R4              * AND RESTORE STACK
         ST    R6,24(R1)          * SAVE VAL IN ENV
         L     R6,=F'1'
         ST    R6,20(R1)          * AND SET LONGJ TO 1.
*        L     R14,16(R1)          * AND RETURN ADDRESS
*        B     RETURNSR            * AND BACK INTO SETJMP
*        L     R15,64(R1)                 RESTORE PSW
*        LM    R0,R15,0(R1)               RESTORE REGISTERS
*        BR    R15                        JUMP TO SAVED PSW
*
RETURNLR DS    0H
         SR    R15,R15            * CLEAR RETURN CODE
         RETURN (14,12),RC=(15)
         LTORG
*
*
*
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
JFCBPTR  DS    F
JFCB     DS    0F
         IEFJFCBN
ZDCBLEN  EQU   *-ZDCBAREA
         DCBD  DSORG=PS
         END
