**********************************************************************  CMS00010
*                                                                    *  CMS00020
*  THIS PROGRAM WRITTEN BY PAUL EDWARDS.                             *  CMS00030
*  RELEASED TO THE PUBLIC DOMAIN                                     *  CMS00040
*                                                                    *  CMS00050
**********************************************************************  CMS00060
**********************************************************************  CMS00070
*                                                                    *  CMS00080
*  CMSSTART - STARTUP ROUTINES FOR CMS FOR USE WITH GCC.             *  CMS00090
*                                                                    *  CMS00100
**********************************************************************  CMS00110
         AIF ('&SYSPARM' EQ 'IFOX00').NOMODE                            CMS00120
         AMODE ANY                                                      CMS00130
         RMODE ANY                                                      CMS00140
.NOMODE ANOP                                                            CMS00150
*        PRINT NOGEN                                                    CMS00160
* YREGS IS NOT AVAILABLE WITH IFOX                                      CMS00170
*         YREGS                                                         CMS00180
R0       EQU   0                                                        CMS00190
R1       EQU   1                                                        CMS00200
R2       EQU   2                                                        CMS00210
R3       EQU   3                                                        CMS00220
R4       EQU   4                                                        CMS00230
R5       EQU   5                                                        CMS00240
R6       EQU   6                                                        CMS00250
R7       EQU   7                                                        CMS00260
R8       EQU   8                                                        CMS00270
R9       EQU   9                                                        CMS00280
R10      EQU   10                                                       CMS00290
R11      EQU   11                                                       CMS00300
R12      EQU   12                                                       CMS00310
R13      EQU   13                                                       CMS00320
R14      EQU   14                                                       CMS00330
R15      EQU   15                                                       CMS00340
SUBPOOL  EQU   0                                                        CMS00350
         CSECT                                                          CMS00360
*@@MVSTRT AMODE 31                                                      CMS00370
*@@MVSTRT RMODE ANY                                                     CMS00380
         ENTRY @@MVSTRT                                                 CMS00390
@@MVSTRT EQU   *                                                        CMS00400
         SAVE  (14,12),,@@MVSTRT                                        CMS00410
         LR    R10,R15                                                  CMS00420
         USING @@MVSTRT,R10                                             CMS00430
         LR    R11,R1                                                   CMS00440
         GETMAIN R,LV=STACKLEN,SP=SUBPOOL                               CMS00450
         ST    R13,4(R1)                                                CMS00460
         ST    R1,8(R13)                                                CMS00470
         LR    R13,R1                                                   CMS00480
         LR    R1,R11                                                   CMS00490
         USING STACK,R13                                                CMS00500
*DW* SAVE STACK POINTER FOR SETJMP/LONGJMP                              CMS00510
         EXTRN @MANSTK@                                                 CMS00520
         L     R3,=V(@MANSTK@)                                          CMS00530
         ST    R13,0(R3)                                                CMS00540
         L     R2,=A(STACKLEN)                                          CMS00550
         ST    R2,4(R3)                                                 CMS00560
*DW END OF MOD                                                          CMS00570
*                                                                       CMS00580
         LA    R2,0                                                     CMS00590
         ST    R2,DUMMYPTR       WHO KNOWS WHAT THIS IS USED FOR        CMS00600
         LA    R2,MAINSTK                                               CMS00610
         ST    R2,THEIRSTK       NEXT AVAILABLE SPOT IN STACK           CMS00620
         LA    R12,ANCHOR                                               CMS00630
         ST    R14,EXITADDR                                             CMS00640
         L     R3,=A(MAINLEN)                                           CMS00650
         AR    R2,R3                                                    CMS00660
         ST    R2,12(R12)        TOP OF STACK POINTER                   CMS00670
         LA    R2,0                                                     CMS00680
         ST    R2,116(R12)       ADDR OF MEMORY ALLOCATION ROUTINE      CMS00690
         ST    R2,ARGPTR                                                CMS00700
*                                                                       CMS00710
         USING NUCON,R0                                                 CMS00720
         MVC   PGMNAME,CMNDLINE                                         CMS00730
         LR    R2,R11                                                   CMS00740
         LA    R2,8(R11)                                                CMS00750
         ST    R2,ARGPTR                                                CMS00760
*                                                                       CMS00770
         LA    R2,PGMNAME                                               CMS00780
         ST    R2,PGMNPTR                                               CMS00790
*                                                                       CMS00800
* FOR GCC WE NEED TO BE ABLE TO RESTORE R13                             CMS00810
         L     R5,SAVEAREA+4                                            CMS00820
         ST    R5,SAVER13                                               CMS00830
*                                                                       CMS00840
         LA    R1,PARMLIST                                              CMS00850
         CALL  @@START                                                  CMS00860
         B     RETURN                                                   CMS00870
*                                                                       CMS00880
RETURN   DS    0H                                                       CMS00890
         LR    R1,R13                                                   CMS00900
         L     R13,SAVEAREA+4                                           CMS00910
         LR    R14,R15                                                  CMS00920
         FREEMAIN R,LV=STACKLEN,A=(R1),SP=SUBPOOL                       CMS00930
         LR    R15,R14                                                  CMS00940
         RETURN (14,12),RC=(15)                                         CMS00950
SAVER13  DS    F                                                        CMS00960
         LTORG                                                          CMS00970
*CEESTART AMODE 31                                                      CMS00980
*CEESTART RMODE ANY                                                     CMS00990
*         ENTRY CEESTART                                                CMS01000
*CEESTART EQU   *                                                       CMS01010
*CEESG003 AMODE 31                                                      CMS01020
*CEESG003 RMODE ANY                                                     CMS01030
*         ENTRY CEESG003                                                CMS01040
*CEESG003 EQU   *                                                       CMS01050
*@@CRT0    AMODE 31                                                     CMS01060
*@@CRT0    RMODE ANY                                                    CMS01070
         ENTRY @@CRT0                                                   CMS01080
@@CRT0   EQU   *                                                        CMS01090
*@@EXITA  AMODE 31                                                      CMS01100
*@@EXITA  RMODE ANY                                                     CMS01110
         ENTRY @@EXITA                                                  CMS01120
@@EXITA  EQU   *                                                        CMS01130
*         L     R14,0(R12)                                              CMS01140
*         L     R15,0(R1)                                               CMS01150
* FOR GCC, WE HAVE TO USE OUR SAVED R13                                 CMS01160
         DROP  R10                                                      CMS01170
         USING @@EXITA,R15                                              CMS01180
         L     R13,=A(SAVER13)                                          CMS01190
         L     R13,0(R13)                                               CMS01200
         L     R15,0(R1)                                                CMS01210
         RETURN (14,12),RC=(15)                                         CMS01220
*         BR    R14                                                     CMS01230
         LTORG                                                          CMS01240
*                                                                       CMS01250
STACK    DSECT                                                          CMS01260
SAVEAREA DS    18F                                                      CMS01270
DUMMYPTR DS    F                                                        CMS01280
THEIRSTK DS    F                                                        CMS01290
PARMLIST DS    0F                                                       CMS01300
ARGPTR   DS    F                                                        CMS01310
PGMNPTR  DS    F                                                        CMS01320
TYPE     DS    F                                                        CMS01330
PGMNAME  DS    CL8                                                      CMS01340
PGMNAMEN DS    C                 NUL BYTE FOR C                         CMS01350
ANCHOR   DS    0F                                                       CMS01360
EXITADDR DS    F                                                        CMS01370
         DS    49F                                                      CMS01380
MAINSTK  DS    16000F                                                   CMS01390
MAINLEN  EQU   *-MAINSTK                                                CMS01400
STACKLEN EQU   *-STACK                                                  CMS01410
         NUCON                                                          CMS01420
         END                                                            CMS01430
                                                                        CMS01440
