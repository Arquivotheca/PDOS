**********************************************************************  CMS00010
*                                                                    *  CMS00020
*  THIS PROGRAM WRITTEN BY PAUL EDWARDS.                             *  CMS00030
*  RELEASED TO THE PUBLIC DOMAIN                                     *  CMS00040
*                                                                    *  CMS00050
**********************************************************************  CMS00060
*                                                                       CMS00070
* MODS FOR DAVE WADE                                                    CMS00080
*                                                                       CMS00090
*                                                                       CMS00100
*   1 - CHANGE REQUEST TYPE FROM RU TO R ON ALL GETMAIN/FREEMAIN        CMS00110
*                                                                       CMS00120
*   2 - REMOVE IEFJFCB AND REPLACE WITH DS 170                          CMS00130
*                                                                       CMS00140
*   3 - ADD SVC 202 ROUTINE TO ALLOW CMS FUNCTIONS TO BE CALLED         CMS00150
*                                                                       CMS00160
**********************************************************************  CMS00170
*                                                                    *  CMS00180
*  MVSSUPA - SUPPORT ROUTINES FOR PDPCLIB UNDER MVS                  *  CMS00190
*                                                                    *  CMS00200
**********************************************************************  CMS00210
         AIF ('&SYSPARM' EQ 'IFOX00').NOMODE                            CMS00220
* BECAUSE OF THE "LOC=ABOVE", WE NEED TO FORCE 31                       CMS00230
* SEARCH FOR "LOC=RES" TO FIND OUT HOW TO FIX                           CMS00240
         AMODE 31                                                       CMS00250
* SEARCH FOR "LOC=RES" TO FIND OUT WHY THIS IS BEING                    CMS00260
* HELD BACK AT RMODE 24                                                 CMS00270
         RMODE 24                                                       CMS00280
.NOMODE ANOP                                                            CMS00290
         PRINT GEN  * WAS NO GEN                                        CMS00300
* YREGS IS NOT AVAILABLE WITH IFOX                                      CMS00310
*         YREGS                                                         CMS00320
R0       EQU   0                                                        CMS00330
R1       EQU   1                                                        CMS00340
R2       EQU   2                                                        CMS00350
R3       EQU   3                                                        CMS00360
R4       EQU   4                                                        CMS00370
R5       EQU   5                                                        CMS00380
R6       EQU   6                                                        CMS00390
R7       EQU   7                                                        CMS00400
R8       EQU   8                                                        CMS00410
R9       EQU   9                                                        CMS00420
R10      EQU   10                                                       CMS00430
R11      EQU   11                                                       CMS00440
R12      EQU   12                                                       CMS00450
R13      EQU   13                                                       CMS00460
R14      EQU   14                                                       CMS00470
R15      EQU   15                                                       CMS00480
SUBPOOL  EQU   0                                                        CMS00490
         CSECT                                                          CMS00500
*@@AOPEN  AMODE 31                                                      CMS00510
*@@AOPEN  RMODE ANY                                                     CMS00520
         ENTRY @@AOPEN                                                  CMS00530
@@AOPEN  EQU   *                                                        CMS00540
         SAVE  (14,12),,@@AOPEN                                         CMS00550
         LR    R12,R15                                                  CMS00560
         USING @@AOPEN,R12                                              CMS00570
         LR    R11,R1                                                   CMS00580
         GETMAIN R,LV=WORKLEN,SP=SUBPOOL                                CMS00590
         ST    R13,4(R1)                                                CMS00600
         ST    R1,8(R13)                                                CMS00610
         LR    R13,R1                                                   CMS00620
         LR    R1,R11                                                   CMS00630
         USING WORKAREA,R13                                             CMS00640
*                                                                       CMS00650
         L     R3,0(R1)         R3 POINTS TO DDNAME                     CMS00660
         L     R4,4(R1)         R4 POINTS TO MODE                       CMS00670
         L     R5,8(R1)         R5 POINTS TO RECFM                      CMS00680
         L     R8,12(R1)        R8 POINTS TO LRECL                      CMS00690
         L     R9,16(R1)        R9 POINTS TO MEMBER NAME (OF PDS)       CMS00700
         AIF   ('&SYSPARM' NE 'IFOX00').BELOW                           CMS00710
* CAN'T USE "BELOW" ON MVS 3.8                                          CMS00720
         GETMAIN R,LV=ZDCBLEN,SP=SUBPOOL                                CMS00730
         AGO   .CHKBLWE                                                 CMS00740
.BELOW   ANOP                                                           CMS00750
         GETMAIN R,LV=ZDCBLEN,SP=SUBPOOL,LOC=BELOW                      CMS00760
.CHKBLWE ANOP                                                           CMS00770
         LR    R2,R1                                                    CMS00780
* THIS LINE IS FOR GCC                                                  CMS00790
         LR    R6,R4                                                    CMS00800
* THIS LINE IS FOR C/370                                                CMS00810
*         L     R6,0(R4)                                                CMS00820
         LTR   R6,R6                                                    CMS00830
         BNZ   WRITING                                                  CMS00840
* READING                                                               CMS00850
         USING ZDCBAREA,R2                                              CMS00860
         MVC   ZDCBAREA(INDCBLN),INDCB                                  CMS00870
* NO NEED TO COPY ON VM/370                                             CMS00880
*        MVC   EOFR24(EOFRLEN),ENDFILE                                  CMS00890
         LA    R10,JFCB                                                 CMS00900
* EXIT TYPE 07 + 80 (END OF LIST INDICATOR)                             CMS00910
         ICM   R10,B'1000',=X'87'                                       CMS00920
         ST    R10,JFCBPTR                                              CMS00930
         LA    R10,JFCBPTR                                              CMS00940
* DW * DID NOT COPY                                                     CMS00950
*        LA    R4,EOFR24                                                CMS00960
         LA    R4,ENDFILE    **** USE ORIGINAL ENDFILE CODE             CMS00970
         USING IHADCB,R2                                                CMS00980
* DW * THIS DOES NOT SEEM TO WORK ON VM/370                             CMS00990
*        STCM  R4,B'0111',DCBEODA                                       CMS01000
*        STCM  R10,B'0111',DCBEXLSA                                     CMS01010
         ST    R4,DCBEODAD                                              CMS01020
         ST    R10,DCBEXLST                                             CMS01030
* END OF MOD                                                            CMS01040
         MVC   DCBDDNAM,0(R3)                                           CMS01050
         MVC   OPENMB,OPENMAC                                           CMS01060
* NOTE THAT THIS IS CURRENTLY NOT REENTRANT AND SHOULD BE MADE SO       CMS01070
         RDJFCB ((R2),INPUT)                                            CMS01080
         LTR   R9,R9                                                    CMS01090
* DW * DON'T SUPPORT MEMBER NAME FOR NOW                                CMS01100
*        BZ    NOMEM                                                    CMS01110
         B     NOMEM                                                    CMS01120
         USING ZDCBAREA,R2                                              CMS01130
*        MVC   JFCBELNM,0(R9)                                           CMS01140
*        OI    JFCBIND1,JFCPDS                                          CMS01150
* DW * END OF MOD                                                       CMS01160
NOMEM    DS    0H                                                       CMS01170
*         OPEN  ((R2),INPUT),MF=(E,OPENMB),MODE=31,TYPE=J               CMS01180
* CAN'T USE MODE=31 ON MVS 3.8, OR WITH TYPE=J                          CMS01190
         OPEN  ((R2),INPUT),MF=(E,OPENMB),TYPE=J                        CMS01200
         B     DONEOPEN                                                 CMS01210
WRITING  DS    0H                                                       CMS01220
         USING ZDCBAREA,R2                                              CMS01230
         MVC   ZDCBAREA(OUTDCBLN),OUTDCB                                CMS01240
         LA    R10,JFCB                                                 CMS01250
* EXIT TYPE 07 + 80 (END OF LIST INDICATOR)                             CMS01260
         ICM   R10,B'1000',=X'87'                                       CMS01270
         ST    R10,JFCBPTR                                              CMS01280
         LA    R10,JFCBPTR                                              CMS01290
         USING IHADCB,R2                                                CMS01300
* DW * DOES NOT SEEM TO WORK ON VM/370                                  CMS01310
*        STCM  R10,B'0111',DCBEXLSA                                     CMS01320
         ST    R10,DCBEXLST                                             CMS01330
* DW * END OF MOD                                                       CMS01340
         MVC   DCBDDNAM,0(R3)                                           CMS01350
         MVC   WOPENMB,WOPENMAC                                         CMS01360
* NOTE THAT THIS IS CURRENTLY NOT REENTRANT AND SHOULD BE MADE SO       CMS01370
         RDJFCB ((R2),OUTPUT)                                           CMS01380
         LTR   R9,R9                                                    CMS01390
* DW * NO MEMBER ON VM/370                                              CMS01400
*        BZ    WNOMEM                                                   CMS01410
         B     WNOMEM                                                   CMS01420
         USING ZDCBAREA,R2                                              CMS01430
*        MVC   JFCBELNM,0(R9)                                           CMS01440
*        OI    JFCBIND1,JFCPDS                                          CMS01450
* DW * END OF MOD                                                       CMS01460
WNOMEM   DS    0H                                                       CMS01470
*         OPEN  ((R2),OUTPUT),MF=(E,WOPENMB),MODE=31,TYPE=J             CMS01480
* CAN'T USE MODE=31 ON MVS 3.8, OR WITH TYPE=J                          CMS01490
         OPEN  ((R2),OUTPUT),MF=(E,WOPENMB),TYPE=J                      CMS01500
DONEOPEN DS    0H                                                       CMS01510
         USING IHADCB,R2                                                CMS01520
         SR    R6,R6                                                    CMS01530
         LH    R6,DCBLRECL                                              CMS01540
         ST    R6,0(R8)                                                 CMS01550
* DW * VM/370 IS MISSING THESE DEFS                                     CMS01560
*        TM    DCBRECFM,DCBRECF                                         CMS01570
         TM    DCBRECFM,RECF                                            CMS01580
* END                                                                   CMS01590
         BNO   VARIABLE                                                 CMS01600
         L     R6,=F'0'                                                 CMS01610
         B     DONESET                                                  CMS01620
VARIABLE DS    0H                                                       CMS01630
         L     R6,=F'1'                                                 CMS01640
DONESET  DS    0H                                                       CMS01650
         ST    R6,0(R5)                                                 CMS01660
         LR    R15,R2                                                   CMS01670
         B     RETURN                                                   CMS01680
*                                                                       CMS01690
* THIS IS NOT EXECUTED DIRECTLY, BUT COPIED INTO 24-BIT STORAGE         CMS01700
ENDFILE  LA    R6,1                                                     CMS01710
* FOLLOWING INSERTED BY DW *                                            CMS01720
         ST    R6,RDEOF                                                 CMS01730
         LR    R15,R6                                                   CMS01740
         ST    R1,0(R3)                                                 CMS01750
*DW END OF MOD                                                          CMS01760
         BR    R14                                                      CMS01770
EOFRLEN  EQU   *-ENDFILE                                                CMS01780
*                                                                       CMS01790
RETURN   DS    0H                                                       CMS01800
         LR    R1,R13                                                   CMS01810
         L     R13,SAVEAREA+4                                           CMS01820
         LR    R14,R15                                                  CMS01830
         FREEMAIN R,LV=WORKLEN,A=(R1),SP=SUBPOOL                        CMS01840
         LR    R15,R14                                                  CMS01850
         RETURN (14,12),RC=(15)                                         CMS01860
         LTORG                                                          CMS01870
* OPENMAC  OPEN  (,INPUT),MF=L,MODE=31                                  CMS01880
* CAN'T USE MODE=31 ON MVS 3.8                                          CMS01890
OPENMAC  OPEN  (,INPUT),MF=L,TYPE=J                                     CMS01900
OPENMLN  EQU   *-OPENMAC                                                CMS01910
* WOPENMAC OPEN  (,OUTPUT),MF=L,MODE=31                                 CMS01920
* CAN'T USE MODE=31 ON MVS 3.8                                          CMS01930
WOPENMAC OPEN  (,OUTPUT),MF=L                                           CMS01940
WOPENMLN EQU   *-WOPENMAC                                               CMS01950
*INDCB    DCB   MACRF=GL,DSORG=PS,EODAD=ENDFILE,EXLST=JPTR              CMS01960
* LEAVE OUT EODAD AND EXLST, FILLED IN LATER                            CMS01970
INDCB    DCB   MACRF=GL,DSORG=PS,EODAD=ENDFILE,EXLST=JPTR               CMS01980
INDCBLN  EQU   *-INDCB                                                  CMS01990
JPTR     DS    F                                                        CMS02000
OUTDCB   DCB   MACRF=PL,DSORG=PS                                        CMS02010
OUTDCBLN EQU   *-OUTDCB                                                 CMS02020
*                                                                       CMS02030
*                                                                       CMS02040
*                                                                       CMS02050
*@@AREAD  AMODE 31                                                      CMS02060
*@@AREAD  RMODE ANY                                                     CMS02070
         ENTRY @@AREAD                                                  CMS02080
@@AREAD  EQU   *                                                        CMS02090
         SAVE  (14,12),,@@AREAD                                         CMS02100
         LR    R12,R15                                                  CMS02110
         USING @@AREAD,R12                                              CMS02120
         LR    R11,R1                                                   CMS02130
         GETMAIN R,LV=WORKLEN,SP=SUBPOOL                                CMS02140
         ST    R13,4(R1)                                                CMS02150
         ST    R1,8(R13)                                                CMS02160
         LR    R13,R1                                                   CMS02170
         LR    R1,R11                                                   CMS02180
         USING WORKAREA,R13                                             CMS02190
*                                                                       CMS02200
         L     R2,0(R1)         R2 CONTAINS HANDLE                      CMS02210
         L     R3,4(R1)         R3 POINTS TO BUF POINTER                CMS02220
         LA    R6,0                                                     CMS02230
         ST    R6,RDEOF                                                 CMS02240
         GET   (R2)                                                     CMS02250
         ST    R1,0(R3)                                                 CMS02260
*        LR    R15,R6                                                   CMS02270
         L     R15,RDEOF                                                CMS02280
         B     RETURN2                                                  CMS02290
*                                                                       CMS02300
RETURN2  DS    0H                                                       CMS02310
         LR    R1,R13                                                   CMS02320
         L     R13,SAVEAREA+4                                           CMS02330
         LR    R14,R15                                                  CMS02340
         FREEMAIN R,LV=WORKLEN,A=(R1),SP=SUBPOOL                        CMS02350
         LR    R15,R14                                                  CMS02360
         RETURN (14,12),RC=(15)                                         CMS02370
*                                                                       CMS02380
*                                                                       CMS02390
*                                                                       CMS02400
*@@AWRITE AMODE 31                                                      CMS02410
*@@AWRITE RMODE ANY                                                     CMS02420
         ENTRY @@AWRITE                                                 CMS02430
@@AWRITE EQU   *                                                        CMS02440
         SAVE  (14,12),,@@AWRITE                                        CMS02450
         LR    R12,R15                                                  CMS02460
         USING @@AWRITE,R12                                             CMS02470
         LR    R11,R1                                                   CMS02480
         GETMAIN R,LV=WORKLEN,SP=SUBPOOL                                CMS02490
         ST    R13,4(R1)                                                CMS02500
         ST    R1,8(R13)                                                CMS02510
         LR    R13,R1                                                   CMS02520
         LR    R1,R11                                                   CMS02530
         USING WORKAREA,R13                                             CMS02540
*                                                                       CMS02550
         L     R2,0(R1)         R2 CONTAINS HANDLE                      CMS02560
         L     R3,4(R1)         R3 POINTS TO BUF POINTER                CMS02570
         PUT   (R2)                                                     CMS02580
         ST    R1,0(R3)                                                 CMS02590
         LA    R15,0                                                    CMS02600
         B     RETURNWR                                                 CMS02610
*                                                                       CMS02620
RETURNWR DS    0H                                                       CMS02630
         LR    R1,R13                                                   CMS02640
         L     R13,SAVEAREA+4                                           CMS02650
         LR    R14,R15                                                  CMS02660
         FREEMAIN R,LV=WORKLEN,A=(R1),SP=SUBPOOL                        CMS02670
         LR    R15,R14                                                  CMS02680
         RETURN (14,12),RC=(15)                                         CMS02690
*                                                                       CMS02700
*                                                                       CMS02710
*                                                                       CMS02720
*@@ACLOSE AMODE 31                                                      CMS02730
*@@ACLOSE RMODE ANY                                                     CMS02740
         ENTRY @@ACLOSE                                                 CMS02750
@@ACLOSE EQU   *                                                        CMS02760
         SAVE  (14,12),,@@ACLOSE                                        CMS02770
         LR    R12,R15                                                  CMS02780
         USING @@ACLOSE,R12                                             CMS02790
         LR    R11,R1                                                   CMS02800
         GETMAIN R,LV=WORKLEN,SP=SUBPOOL                                CMS02810
         ST    R13,4(R1)                                                CMS02820
         ST    R1,8(R13)                                                CMS02830
         LR    R13,R1                                                   CMS02840
         LR    R1,R11                                                   CMS02850
         USING WORKAREA,R13                                             CMS02860
*                                                                       CMS02870
         L     R2,0(R1)         R2 CONTAINS HANDLE                      CMS02880
         MVC   CLOSEMB,CLOSEMAC                                         CMS02890
*         CLOSE ((R2)),MF=(E,CLOSEMB),MODE=31                           CMS02900
* CAN'T USE MODE=31 WITH MVS 3.8                                        CMS02910
         CLOSE ((R2)),MF=(E,CLOSEMB)                                    CMS02920
         FREEPOOL ((R2))                                                CMS02930
         FREEMAIN R,LV=ZDCBLEN,A=(R2),SP=SUBPOOL                        CMS02940
         LA    R15,0                                                    CMS02950
         B     RETURN3                                                  CMS02960
*                                                                       CMS02970
RETURN3  DS    0H                                                       CMS02980
         LR    R1,R13                                                   CMS02990
         L     R13,SAVEAREA+4                                           CMS03000
         LR    R14,R15                                                  CMS03010
         FREEMAIN R,LV=WORKLEN,A=(R1),SP=SUBPOOL                        CMS03020
         LR    R15,R14                                                  CMS03030
         RETURN (14,12),RC=(15)                                         CMS03040
         LTORG                                                          CMS03050
* CLOSEMAC CLOSE (),MF=L,MODE=31                                        CMS03060
* CAN'T USE MODE=31 WITH MVS 3.8                                        CMS03070
CLOSEMAC CLOSE (),MF=L                                                  CMS03080
CLOSEMLN EQU   *-CLOSEMAC                                               CMS03090
*                                                                       CMS03100
*                                                                       CMS03110
*                                                                       CMS03120
**********************************************************************  CMS03130
*                                                                    *  CMS03140
*  GETM - GET MEMORY                                                 *  CMS03150
*                                                                    *  CMS03160
**********************************************************************  CMS03170
*@@GETM   AMODE 31                                                      CMS03180
*@@GETM   RMODE ANY                                                     CMS03190
         ENTRY @@GETM                                                   CMS03200
@@GETM   EQU   *                                                        CMS03210
         SAVE  (14,12),,@@GETM                                          CMS03220
         LR    R12,R15                                                  CMS03230
         USING @@GETM,R12                                               CMS03240
*                                                                       CMS03250
         L     R2,0(R1)                                                 CMS03260
* THIS LINE IS FOR GCC                                                  CMS03270
         LR    R3,R2                                                    CMS03280
* THIS LINE IS FOR C/370                                                CMS03290
*         L     R3,0(R2)                                                CMS03300
         A     R3,=F'16'                                                CMS03310
*                                                                       CMS03320
* THIS SHOULD NOT BE NECESSARY. THE DEFAULT OF LOC=RES                  CMS03330
* SHOULD BE SUFFICIENT. HOWEVER, CURRENTLY THERE IS AN                  CMS03340
* UNKNOWN PROBLEM, PROBABLY WITH RDJFCB, WHICH PREVENTS                 CMS03350
* EXECUTABLES FROM RESIDING ABOVE THE LINE, HENCE THIS                  CMS03360
* HACK TO ALLOCATE MOST STORAGE ABOVE THE LINE                          CMS03370
*                                                                       CMS03380
         AIF   ('&SYSPARM' NE 'IFOX00').ANYCHKY                         CMS03390
* CAN'T USE "ANY" ON MVS 3.8                                            CMS03400
         GETMAIN R,LV=(R3),SP=SUBPOOL                                   CMS03410
         AGO   .ANYCHKE                                                 CMS03420
.ANYCHKY ANOP                                                           CMS03430
         GETMAIN R,LV=(R3),SP=SUBPOOL,LOC=ANY                           CMS03440
.ANYCHKE ANOP                                                           CMS03450
*                                                                       CMS03460
         ST    R3,0(R1)                                                 CMS03470
         A     R1,=F'16'                                                CMS03480
         LR    R15,R1                                                   CMS03490
*                                                                       CMS03500
RETURNGM DS    0H                                                       CMS03510
         RETURN (14,12),RC=(15)                                         CMS03520
         LTORG                                                          CMS03530
*                                                                       CMS03540
*                                                                       CMS03550
*                                                                       CMS03560
**********************************************************************  CMS03570
*                                                                    *  CMS03580
*  FREEM - FREE MEMORY                                               *  CMS03590
*                                                                    *  CMS03600
**********************************************************************  CMS03610
*@@FREEM  AMODE 31                                                      CMS03620
*@@FREEM  RMODE ANY                                                     CMS03630
         ENTRY @@FREEM                                                  CMS03640
@@FREEM  EQU   *                                                        CMS03650
         SAVE  (14,12),,@@FREEM                                         CMS03660
         LR    R12,R15                                                  CMS03670
         USING @@FREEM,R12                                              CMS03680
*                                                                       CMS03690
         L     R2,0(R1)                                                 CMS03700
         S     R2,=F'16'                                                CMS03710
         L     R3,0(R2)                                                 CMS03720
         FREEMAIN R,LV=(R3),A=(R2),SP=SUBPOOL                           CMS03730
*                                                                       CMS03740
RETURNFM DS    0H                                                       CMS03750
         RETURN (14,12),RC=(15)                                         CMS03760
         LTORG                                                          CMS03770
*                                                                       CMS03780
*                                                                       CMS03790
*                                                                       CMS03800
**********************************************************************  CMS03810
*                                                                       CMS03820
*  @SVC202@ - ISSUES AN SVC 202 CALL                                    CMS03830
*                                                                       CMS03840
*  E.G. @SVC202@(PARMS,CODE,ERROR)                                      CMS03850
*                                                                       CMS03860
* WHERE :-                                                              CMS03870
*                                                                       CMS03880
*  PARMS IS A POINTER TO AN SVC202 PARAMETER LIST                       CMS03890
*                                                                       CMS03900
*  CODE IS A CODE TO SAY OF &CONTROL IS ON OR OFF                       CMS03910
*                                                                       CMS03920
* AND ERROR IS SET TO -1                                                CMS03930
*                                                                       CMS03940
**********************************************************************  CMS03950
         ENTRY @SVC202@                                                 CMS03960
@SVC202@ EQU *                                                          CMS03970
         SAVE  (14,12),,@SVC202@                                        CMS03980
         LR    R12,R15                                                  CMS03990
         USING @SVC202@,R12                                             CMS04000
         LR    R11,R1           NEED TO RESTORE R1 FOR C                CMS04010
         L     R3,0(R1)         R3 POINTS TO SVC202 PARM LIST           CMS04020
         L     R4,4(R1)         R4 POINTS TO CODE                       CMS04030
         L     R5,8(R1)         R5 POINTS TO RETURN CODE                CMS04040
         SR    R6,R6            CLEAR R6                                CMS04050
         ST    R6,0(R5)         AND SAVE IN RETURN CODE                 CMS04060
         LR    R1,R3                                                    CMS04070
         SVC   202              ISSUE COMMAND                           CMS04080
         DC    AL4(SV202ER)      ERROR                                  CMS04090
SV202RT  EQU    *                                                       CMS04100
         LR     R1,R11                                                  CMS04110
         RETURN (14,12),RC=(15)                                         CMS04120
SV202ER  EQU    *                                                       CMS04130
         L      R3,=F'-1'                                               CMS04140
         ST     R3,0(R5)                                                CMS04150
         B      SV202RT                                                 CMS04160
         LTORG                                                          CMS04170
*                                                                       CMS04180
*                                                                       CMS04190
*                                                                       CMS04200
**********************************************************************  CMS04210
*                                                                       CMS04220
*  @@ATTN@@ - ISSUES AN SVC 202 CALL TO STACK A LINE                    CMS04230
*                                                                       CMS04240
*  E.G. @SVC202@(LINE,LEN,ORDER)                                        CMS04250
*                                                                       CMS04260
* WHERE :-                                                              CMS04270
*                                                                       CMS04280
*  LINE IS A POINTER TO LINE TO BE STACKED                              CMS04290
*                                                                       CMS04300
*  LEN IS THE NUMBER OF CHARACTERS. (<256)                              CMS04310
*                                                                       CMS04320
*  ORDER IS POINTER TO EITHER FIFO OR LIFI                              CMS04330
*                                                                       CMS04340
**********************************************************************  CMS04350
         ENTRY @@ATTN@@                                                 CMS04360
@@ATTN@@ EQU *                                                          CMS04370
         SAVE  (14,12),,@@ATTN@@                                        CMS04380
         LR    R12,R15                                                  CMS04390
         USING @@ATTN@@,R12                                             CMS04400
         LR    R11,R1           NEED TO RESTORE R1 FOR C                CMS04410
         L     R3,0(R1)         R3 POINTS TO LINE TO STACK              CMS04420
         ST    R3,ATTNLN        SAVE IN 202 PLIST                       CMS04430
         L     R4,4(R1)         R4 POINTS TO LENGTH OF LINE             CMS04440
         MVC   ATTNLN,3(R4)     FIDDLE                                  CMS04450
         L     R5,8(R1)         R5 POINTS TO LIFO OR FIFO               CMS04460
         MVC   ATTNOD,0(R5)                                             CMS04470
         SR    R6,R6            CLEAR R6                                CMS04480
*        ST    R6,0(R5)         AND SAVE IN RETURN CODE                 CMS04490
         LA    R1,ATTNPL                                                CMS04500
         SVC   202              ISSUE COMMAND                           CMS04510
         DC    AL4(ATTNER)      ERROR                                   CMS04520
ATTNRT   EQU    *                                                       CMS04530
         LR     R1,R11                                                  CMS04540
         RETURN (14,12),RC=(15)                                         CMS04550
ATTNER   EQU    *                                                       CMS04560
*        L      R3,=F'-1'                                               CMS04570
*        ST     R3,0(R5)                                                CMS04580
         B      ATTNRT                                                  CMS04590
         LTORG                                                          CMS04600
*                                                                       CMS04610
ATTNPL   DS   0D                                                        CMS04620
         DC   CL8'ATTN'                                                 CMS04630
ATTNOD   DC   CL4'XXXX'     WHERE ORDER MAY BE LIFO OR FIFO.            CMS04640
*                            FIFO IS THE DEFAULT                        CMS04650
ATTNLN   DC   AL1(0)         LENGTH OF LINE TO BE STACKED               CMS04660
ATTNAD   DC   AL3(ATTNAD)    ADDRESS OF LINE TO BE STACKED              CMS04670
*                                                                       CMS04680
*                                                                       CMS04690
**********************************************************************  CMS04700
*                                                                       CMS04710
*  @STACKN@ - RETURNS THE NUMBER OF LINES ON THE CONSOLE STACK          CMS04720
*                                                                       CMS04730
*  E.G. @STACKN@(COUNT)                                                 CMS04740
*                                                                       CMS04750
* WHERE :-                                                              CMS04760
*                                                                       CMS04770
*  COUNT IS A POINTER TO AN INT - NUMBER OF LINES TETURNED              CMS04780
*                                                                       CMS04790
*                                                                       CMS04800
**********************************************************************  CMS04810
         ENTRY @STACKN@                                                 CMS04820
@STACKN@ EQU *                                                          CMS04830
         SAVE  (14,12),,@STACKN@                                        CMS04840
         LR    R12,R15                                                  CMS04850
         USING @STACKN@,R12                                             CMS04860
         USING NUCON,R0                                                 CMS04870
         LR    R11,R1           NEED TO RESTORE R1 FOR C                CMS04880
         L     R3,0(R1)         R3 POINTS TO COUNT                      CMS04890
         LH    R2,NUMFINRD      R2 HAS COUNT OF LINES ON STACK          CMS04900
         ST    R2,0(R3)         R2 TO COUNT                             CMS04910
         LR    R1,R11                                                   CMS04920
         RETURN (14,12),RC=(15)                                         CMS04930
         LTORG                                                          CMS04940
*                                                                       CMS04950
**********************************************************************  CMS04960
*                                                                    *  CMS04970
*  GETCLCK - GET THE VALUE OF THE MVS CLOCK TIMER AND MOVE IT TO AN  *  CMS04980
*  8-BYTE FIELD.  THIS 8-BYTE FIELD DOES NOT NEED TO BE ALIGNED IN   *  CMS04990
*  ANY PARTICULAR WAY.                                               *  CMS05000
*                                                                    *  CMS05010
*  E.G. CALL 'GETCLCK' USING WS-CLOCK1                               *  CMS05020
*                                                                    *  CMS05030
*  THIS FUNCTION ALSO RETURNS THE NUMBER OF SECONDS SINCE 1970-01-01 *  CMS05040
*  BY USING SOME EMPERICALLY-DERIVED MAGIC NUMBERS                   *  CMS05050
*                                                                    *  CMS05060
**********************************************************************  CMS05070
*@@GETCLK AMODE 31                                                      CMS05080
*@@GETCLK RMODE ANY                                                     CMS05090
         ENTRY @@GETCLK                                                 CMS05100
@@GETCLK EQU   *                                                        CMS05110
         SAVE  (14,12),,@@GETCLK                                        CMS05120
         LR    R12,R15                                                  CMS05130
         USING @@GETCLK,R12                                             CMS05140
*                                                                       CMS05150
         L     R2,0(R1)                                                 CMS05160
         STCK  0(R2)                                                    CMS05170
         L     R4,0(R2)                                                 CMS05180
         L     R5,4(R2)                                                 CMS05190
         SRDL  R4,12                                                    CMS05200
         SL    R4,=X'0007D910'                                          CMS05210
         D     R4,=F'1000000'                                           CMS05220
         SL    R5,=F'1220'                                              CMS05230
         LR    R15,R5                                                   CMS05240
*                                                                       CMS05250
RETURNGC DS    0H                                                       CMS05260
         RETURN (14,12),RC=(15)                                         CMS05270
         LTORG                                                          CMS05280
*@@SAVER AMODE 31                                                       CMS05290
*@@SAVER RMODE ANY                                                      CMS05300
         ENTRY @@SAVER                                                  CMS05310
@@SAVER EQU   *                                                         CMS05320
*                                                                       CMS05330
         SAVE  (14,12),,SETJMP     * SAVE REGS AS NORMAL                CMS05340
         LR    R12,R15                                                  CMS05350
         USING @@SAVER,12                                               CMS05360
         L     R1,0(R1)            * ADDRESS OF ENV TO R1               CMS05370
         L     R2,@MANSTK@         * R2 POINTS TO START OF STACK        CMS05380
         L     R3,@MANSTK@+4       * R3 HAS LENGTH OF STACK             CMS05390
         LR    R5,R3               * AND R5                             CMS05400
         LR    R9,R1               * R9 NOW CONATINS ADDRESS OF ENV     CMS05410
         GETMAIN R,LV=(R3),SP=SUBPOOL    * GET A SAVE AREA              CMS05420
         ST    R1,0(R9)            * SAVE IT IN FIRST WORK OF ENV       CMS05430
         ST    R5,4(R9)            * SAVE LENGTH IN SECOND WORD OF ENV  CMS05440
         ST    R2,8(R9)            * NOTE WHERE WE GOT IT FROM          CMS05450
         ST    R13,12(R9)          * AND R13                            CMS05460
         LR    R4,R1               * AND R4                             CMS05470
         MVCL  R4,R2               * COPY SETJMP'S SAVE AREA TO ENV     CMS05480
*        STM   R0,R15,0(R1)               SAVE REGISTERS                CMS05490
*        BALR  R15,0                     GET PSW INTO R15               CMS05500
*        ST    R15,64(R1)                 SAVE PSW                      CMS05510
*                                                                       CMS05520
RETURNSR DS    0H                                                       CMS05530
         SR    R15,R15              * CLEAR RETURN CODE                 CMS05540
         RETURN (14,12),RC=(15)                                         CMS05550
         ENTRY   @MANSTK@                                               CMS05560
@MANSTK@ DS    2F                                                       CMS05570
         LTORG                                                          CMS05580
*                                                                       CMS05590
*                                                                       CMS05600
*                                                                       CMS05610
**********************************************************************  CMS05620
*                                                                    *  CMS05630
*  LOADREGS - LOAD REGISTERS AND PSW FROM ENV_BUF                    *  CMS05640
*                                                                    *  CMS05650
**********************************************************************  CMS05660
*@@LOADR AMODE 31                                                       CMS05670
*@@LOADR RMODE ANY                                                      CMS05680
         ENTRY @@LOADR                                                  CMS05690
@@LOADR EQU   *                                                         CMS05700
*                                                                       CMS05710
         BALR  R12,R0                                                   CMS05720
         USING *,12                                                     CMS05730
         L     R1,0(R1)           * R1 POINTS TO ENV                    CMS05740
         L     R2,8(R1)           * R2 POINTS TO STACK                  CMS05750
         L     R3,4(R1)           * R3 HAS HOW LONG                     CMS05760
         LR    R5,R3              * AS DOES R5                          CMS05770
         L     R6,24(R1)          * R6 HAS RETURN CODE                  CMS05780
         L     R4,0(R1)           * OUR SAVE AREA                       CMS05790
         L     R13,12(R1)         * GET OLD STACK POINTER               CMS05800
         MVCL  R2,R4              * AND RESTORE STACK                   CMS05810
         ST    R6,24(R1)          * SAVE VAL IN ENV                     CMS05820
         L     R6,=F'1'                                                 CMS05830
         ST    R6,20(R1)          * AND SET LONGJ TO 1.                 CMS05840
*        L     R14,16(R1)          * AND RETURN ADDRESS                 CMS05850
*        B     RETURNSR            * AND BACK INTO SETJMP               CMS05860
*        L     R15,64(R1)                 RESTORE PSW                   CMS05870
*        LM    R0,R15,0(R1)               RESTORE REGISTERS             CMS05880
*        BR    R15                        JUMP TO SAVED PSW             CMS05890
*                                                                       CMS05900
RETURNLR DS    0H                                                       CMS05910
         SR    R15,R15            * CLEAR RETURN CODE                   CMS05920
         RETURN (14,12),RC=(15)                                         CMS05930
         LTORG                                                          CMS05940
*                                                                       CMS05950
*                                                                       CMS05960
*                                                                       CMS05970
WORKAREA DSECT                                                          CMS05980
SAVEAREA DS    18F                                                      CMS05990
         DS    0F                                                       CMS06000
CLOSEMB  DS    CL(CLOSEMLN)                                             CMS06010
         DS    0F                                                       CMS06020
OPENMB   DS    CL(OPENMLN)                                              CMS06030
         DS    0F                                                       CMS06040
WOPENMB  DS    CL(WOPENMLN)                                             CMS06050
RDEOF    DS    1F                                                       CMS06060
WORKLEN  EQU   *-WORKAREA                                               CMS06070
ZDCBAREA DSECT                                                          CMS06080
         DS    CL(INDCBLN)                                              CMS06090
         DS    CL(OUTDCBLN)                                             CMS06100
         DS    0H                                                       CMS06110
EOFR24   DS    CL(EOFRLEN)                                              CMS06120
JFCBPTR  DS    F                                                        CMS06130
JFCB     DS    0F                                                       CMS06140
*        IEFJFCBN                                                       CMS06150
         DS    CL170                                                    CMS06160
ZDCBLEN  EQU   *-ZDCBAREA                                               CMS06170
         DCBD  DSORG=PS                                                 CMS06180
RECF     EQU   X'80'                   FIXED RECORD FORMAT              CMS06190
RECV     EQU   X'40'                   VARYING RECORD FORMAT            CMS06200
RECU     EQU   X'C0'                   UNDEFINED RECORD FORMAT          CMS06210
RECUV    EQU   X'40'                   U OR V RECORD FORMAT             CMS06220
RECUF    EQU   X'80'                   U OR F RECORD FORMAT             CMS06230
         NUCON                                                          CMS06240
         END                                                            CMS06250
                                                                        CMS06260
