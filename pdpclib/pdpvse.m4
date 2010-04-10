* $$ JOB JNM=PAULPAUL
* $$ LST LST=SYSLST,CLASS=A
// JOB PAULPAUL
*
*
// OPTION CATAL
   PHASE $JOBEXIT,*,SVA
// EXEC ASSEMBLY
R0       EQU       0                                                    $JO00050
R1       EQU       1                                                    $JO00060
R2       EQU       2                                                    $JO00070
R3       EQU       3                                                    $JO00080
R4       EQU       4                                                    $JO00090
R5       EQU       5                                                    $JO00100
R6       EQU       6                                                    $JO00110
R7       EQU       7                                                    $JO00120
R8       EQU       8                                                    $JO00130
R9       EQU       9                                                    $JO00140
R10      EQU       10                                                   $JO00150
R11      EQU       11                                                   $JO00160
R12      EQU       12                                                   $JO00170
R13      EQU       13                                                   $JO00180
R14      EQU       14                                                   $JO00190
R15      EQU       15                                                   $JO00200
*                                                                       $JO00210
TABPART  DSECT                                                          $JO00220
         USING     *,R11                                                $JO00230
PARMPOS  DS        F                                                    $JO00240
PARTPIK  DS        H                                                    $JO00250
PGM      DS        CL8                                                  $JO00260
PARMLEN  DS        H                                                    $JO00270
PARM     DS        CL100                                                $JO00280
TABLEN   EQU       *-TABPART                                            $JO00290
*                                                                       $JO00300
DIALOG   CSECT                                                          $JO00310
         DC        A(ARRAY)                                             $JO00320
$JOBEXIT CSECT                                                          $JO00330
         BALR      12,0                                                 $JO00340
         USING     *,12                                                 $JO00350
*                                                  -->MILLON   22/03/10 $JO00360
         CLC       =C'EXEC',0(R3)            / IS IT AN EXEC STMT       $JO00370
         BNE       RETURN                    / NO RETURN                $JO00380
         LA        R15,71                                               $JO00390
         LR        R8,R4                                                $JO00400
LOOP1    DS        0H                                                   $JO00410
         CLC       =C',PARM=''',0(R8)        / THE STMT CONTAINS PARM   $JO00420
         BE        PROCESS                   / YES PROCESS IT           $JO00430
         LA        R8,1(R8)                                             $JO00440
         BCT       R15,LOOP1                                            $JO00450
RETURN   DS        0H                                                   $JO00460
         SR        R15,R15                                              $JO00470
         BR        R14                                                  $JO00480
PROCESS  DS        0H                                                   $JO00490
         LA        R11,ARRAY                                            $JO00500
         L         R15,20                                               $JO00510
* * * * * AFTER SUCCESSFUL TEST REMOVE 2 FOLLOWING CARDS                $JO00520
         CLC       =C'PAULPAUL',24(R15)      / DURING TEST PERIOD       $JO00530
         BNE       RETURN                    / PROCESS ONLY JOB NAMED   $JO00540
*                                            / PAULPAUL                 $JO00550
         LH        R0,46(R15)                                           $JO00560
         LA        R15,12                                               $JO00570
LOOP0    DS        0H                                                   $JO00580
         CH        R0,PARTPIK                                           $JO00590
         BE        FOUND                                                $JO00600
         LA        R11,TABLEN(R11)                                      $JO00610
         BCT       R15,LOOP0                                            $JO00620
         B         RETURN                                               $JO00630
*                                                                       $JO00640
FOUND    DS        0H                                                   $JO00650
         LA        R8,7(R8)                  / SKIP PARAMETER           $JO00660
         ST        R8,PARMPOS                / REMIND PARM POSITION     $JO00670
         LA        R15,71(R4)                                           $JO00680
         SR        R15,R8                                               $JO00690
LOOP2    DS        0H                                                   $JO00700
         CLI       0(R8),C''''                                          $JO00710
         BE        FINPARM                                              $JO00720
         LA        R8,1(R8)                                             $JO00730
         BCT       R15,LOOP2                                            $JO00740
         B         RETURN                    / INVALID STATEMENT        $JO00750
FINPARM  DS        0H                                                   $JO00760
         LR        R0,R8                                                $JO00770
         S         R0,PARMPOS                                           $JO00780
         STH       R0,PARMLEN                                           $JO00790
         L         R15,PARMPOS                                          $JO00800
         LR        R8,R0                                                $JO00810
         BCTR      R8,0                                                 $JO00820
         EX        R8,MVCPARM                                           $JO00830
*                                                                       $JO00840
         LA        R15,71                                               $JO00850
         LR        R8,R4                                                $JO00860
LOOP4    DS        0H                                                   $JO00870
         CLC       =C' EXEC ',0(R8)                                     $JO00880
         BE        OKEXEC                                               $JO00890
         LA        R8,1(R8)                                             $JO00900
         BCT       R15,LOOP4                                            $JO00910
         B         RETURN                    / INVALID STATEMENT        $JO00920
OKEXEC   DS        0H                                                   $JO00930
         LA        R8,6(R8)                                             $JO00940
         LA        R10,PGM                                              $JO00950
         MVC       PGM,=CL8' '                                          $JO00960
LOOP5    DS        0H                                                   $JO00970
         CLI       0(R8),C' '                                           $JO00980
         BNE       OKPGM                                                $JO00990
         LA        R8,1(R8)                                             $JO01000
         B         LOOP5                                                $JO01010
OKPGM    DS        0H                                                   $JO01020
         CLI       0(R8),C','                                           $JO01030
         BE        FINPGM                                               $JO01040
         MVC       0(1,R10),0(R8)                                       $JO01050
         LA        R10,1(R10)                                           $JO01060
         LA        R8,1(R8)                                             $JO01070
         B         OKPGM                                                $JO01080
FINPGM   DS        0H                                                   $JO01090
         MVC       0(80,R4),=CL80'// EXEC PAULPGM'                      $JO01100
         B         RETURN                                               $JO01110
MVCPARM  MVC       PARM(0),0(R15)                                       $JO01120
         LTORG                                                          $JO01130
         DC        C'EYE CATCHER'                                       $JO01140
ARRAY    DC        F'0',X'00C0',CL8' ',H'0',CL100' '                    $JO01150
         DC        F'0',X'00B0',CL8' ',H'0',CL100' '                    $JO01160
         DC        F'0',X'00A0',CL8' ',H'0',CL100' '                    $JO01170
         DC        F'0',X'0090',CL8' ',H'0',CL100' '                    $JO01180
         DC        F'0',X'0080',CL8' ',H'0',CL100' '                    $JO01190
         DC        F'0',X'0070',CL8' ',H'0',CL100' '                    $JO01200
         DC        F'0',X'0060',CL8' ',H'0',CL100' '                    $JO01210
         DC        F'0',X'0050',CL8' ',H'0',CL100' '                    $JO01220
         DC        F'0',X'0040',CL8' ',H'0',CL100' '                    $JO01230
         DC        F'0',X'0030',CL8' ',H'0',CL100' '                    $JO01240
         DC        F'0',X'0020',CL8' ',H'0',CL100' '                    $JO01250
         DC        F'0',X'0010',CL8' ',H'0',CL100' '                    $JO01260
         DC        H'-1'                                                $JO01270
         END       $JOBEXIT                                             $JO01280
/*                                                                      $JO01290
// EXEC LNKEDT
/*
*
// OPTION CATAL
   PHASE PAULPGM,*
// EXEC ASSEMBLY
R0       EQU       0                                                    PAU00010
R1       EQU       1                                                    PAU00020
R2       EQU       2                                                    PAU00030
R3       EQU       3                                                    PAU00040
R4       EQU       4                                                    PAU00050
R5       EQU       5                                                    PAU00060
R6       EQU       6                                                    PAU00070
R7       EQU       7                                                    PAU00080
R8       EQU       8                                                    PAU00090
R9       EQU       9                                                    PAU00100
R10      EQU       10                                                   PAU00110
R11      EQU       11                                                   PAU00120
R12      EQU       12                                                   PAU00130
R13      EQU       13                                                   PAU00140
R14      EQU       14                                                   PAU00150
R15      EQU       15                                                   PAU00160
*                                                                       PAU00170
TABPART  DSECT                                                          PAU00180
         USING     *,R11                                                PAU00190
PARMPOS  DS        F                                                    PAU00200
PARTPIK  DS        H                                                    PAU00210
PGM      DS        CL8                                                  PAU00220
PARMLEN  DS        H                                                    PAU00230
PARM     DS        CL100                                                PAU00240
TABLEN   EQU       *-TABPART                                            PAU00250
*                                                                       PAU00260
PAULPGM  CSECT                                                          PAU00270
         BALR      R12,0                                                PAU00280
         USING     *,R12                                                PAU00290
*                                                  -->MILLON   22/03/10 PAU00300
         CDLOAD    $JOBEXIT                                             PAU00310
         LR        R11,R1                                               PAU00320
         SH        R11,=H'8'                                            PAU00330
         L         R11,0(R11)                                           PAU00340
         L         R4,20                                                PAU00350
         LH        R4,46(R4)                                            PAU00360
         LA        R15,12                                               PAU00370
LOOP1    DS        0H                                                   PAU00380
         CH        R4,PARTPIK                                           PAU00390
         BE        OKPIK                                                PAU00400
         LA        R11,TABLEN(R11)                                      PAU00410
         BCT       R15,LOOP1                                            PAU00420
         MVC       MSG,=CL60'PIK NOT FOUND'                             PAU00430
         EXCP      CCB                                                  PAU00440
         WAIT      CCB                                                  PAU00450
         DC        H'0'                                                 PAU00460
OKPIK    DS        0H                                                   PAU00470
         LA        R15,PGM                                              PAU00480
         CDLOAD    (R15)                                                PAU00490
         LR        R15,R1                                               PAU00500
         LA        R1,PARMLEN                                           PAU00510
         ST        R1,PARMLIST                                          PAU00520
         LA        R1,PARMLIST                                          PAU00530
         CALL      (15)                                                 PAU00540
         EOJ                                                            PAU00550
PARMLIST DS        F                                                    PAU00560
MSG      DC        CL60' '                                              PAU00570
CCB      CCB       SYSLOG,CCW                                           PAU00580
CCW      CCW       X'01',MSG,0,60                                       PAU00590
         LTORG                                                          PAU00600
         END       PAULPGM                                              PAU00610
/*                                                                      $JO01290
// EXEC LNKEDT
/*
*
* Create a dummy sequential file
*
// DLBL UOUT,'TEST.F80'
// EXTENT SYS005,WORK01,,,13,6
// ASSGN SYS005,DISK,VOL=WORK01,SHR
// ASSGN SYS004,SYSIPT
// EXEC PGM=OBJMAINT,SIZE=AUTO
./ CARD DLM=$$
./ COPY
LINE ABC1
LINE DEF2
$$
/*
*
* Now print the sequential file
*
// ASSGN SYS005,SYSPCH
// ASSGN SYS004,DISK,VOL=WORK01,SHR
// DLBL UIN,'TEST.F80'
// EXTENT SYS004,WORK01
// EXEC PGM=OBJMAINT,SIZE=AUTO
./ LIST
./ COPY
/*
/&
* $$ EOJ
*
* Get the direct macros in
*
*
// OPTION EDECK,NODECK  
// DLBL IJSYSPH,'WORK.FILE',0,SD
// EXTENT SYSPCH,WORK01,1,0,3,6
ASSGN SYSPCH,DISK,VOL=WORK01,SHR
// EXEC ASSEMBLY
*
* Dummy AMODE macro
*
         MACRO
&N       AMODE
         MEND
*
* Dummy RMODE macro
*
         MACRO
&N       RMODE
         MEND
*
* BSM instruction as a macro
*
         MACRO
&LABEL   BSM   &R1,&R2
         DS    0H
&LABEL.  DC    0XL2'00',X'0B',AL.4(&R1.,&R2.)
         MEND
*
* RETURN macro from MVS 3.8j
*
         MACRO
&NAME    RETURN &REG,&PARA,&RC=O
         LCLA  &A
         AIF   ('&NAME' EQ '').GO
&NAME    DS    0H
.GO      AIF   ('&REG' EQ '').CONTA
         AIF   ('&RC' EQ '(15)').SPECRT
.COMBACK ANOP
&A       SETA  &REG(1)*4+20
         AIF   (&A LE 75).CONTB
&A       SETA  &A-64
.CONTB   AIF   (N'&REG NE 2).CONTC
         LM    &REG(1),&REG(2),&A.(13)           RESTORE THE REGISTERS
         AGO   .CONTA
.SPECRT  AIF   ('&REG(1)' NE '14' AND '&REG(1)' NE '15').COMBACK
         AIF   ('&REG(1)' EQ '14' AND N'&REG  EQ 1).COMBACK
         AIF   ('&REG(1)' EQ '15' AND N'&REG EQ 1).CONTA
         AIF   ('&REG(1)' EQ '14').SKIP
         AIF   ('&REG(2)' EQ '0').ZTWO
.LM      LM    0,&REG(2),20(13)                  RESTORE THE REGISTERS
         AGO   .CONTA
.ZTWO    L     0,20(13,0)                        RESTORE REGISTER ZERO
         AGO   .CONTA
.SKIP    L     14,12(13,0)                       RESTORE REGISTER 14
         AIF   ('&REG(2)' EQ '15').CONTA
         AIF   ('&REG(2)' EQ '0').ZTWO
         AGO   .LM
.CONTC   AIF   (N'&REG NE 1).ERROR1
         L     &REG(1),&A.(13,0)                 RESTORE REGISTER
.CONTA   AIF   ('&PARA' EQ '').CONTD
         AIF   ('&PARA' NE 'T').ERROR2
         MVI   12(13),X'FF'                      SET RETURN INDICATION
.CONTD   AIF   ('&RC' EQ 'O').CONTE
         AIF   ('&RC'(1,1) EQ '(').ISAREG
         LA    15,&RC.(0,0)                      LOAD RETURN CODE
         AGO   .CONTE
.ISAREG  AIF   ('&RC(1)' EQ '15').CONTE
         IHBERMAC 61,,&RC
         MEXIT
.CONTE   BR    14                                RETURN
         AGO   .END
.ERROR1  IHBERMAC 36,,&REG
         MEXIT
.ERROR2  IHBERMAC 37,,&PARA
.END     MEND
*
* GETVIS macro from DOS/VS, modified to support LOC=ANY
*
         MACRO
&NAME GETVIS &ERROR,&ADDRESS=,&LENGTH=,&PAGE=NO,&POOL=NO,&SVA=NO,&LOC=
 LCLB &BLPAGE,&BLPOOL,&BLSVA,&BLLOC
* AM/VS ACCESS METHOD - GETVIS - 5745-SC-SUP - REL.29.0
         AIF   (T'&ERROR EQ 'O').G0
         MNOTE 4,'POSITIONAL PARAMETER SPECIFIED - MACRO IGNORED'
         MEXIT
.G0      ANOP
         AIF   (T'&NAME EQ 'O').G1
&NAME    DS    0H
.G1      ANOP
         AIF   (T'&LENGTH EQ 'O').G4
         AIF   ('&LENGTH'(1,1) NE '(').G2
         AIF   (T'&LENGTH(1) NE 'N').G15
         AIF   (&LENGTH(1) EQ 0).G4
.G15     ANOP
         LR    0,&LENGTH(1)       LOAD LENGTH INTO REGISTER 0
         AGO   .G4
.G2      ANOP
         AIF   (T'&LENGTH NE 'N').G3
         MNOTE 4,'LENGTH PARAMETER INVALID  - MACRO IGNORED'
         MEXIT
.G3      ANOP
         L     0,&LENGTH          LOAD LENGTH INTO REGISTER 0
.G4      ANOP
         AIF   ('&PAGE' EQ 'NO').G42
         AIF   ('&PAGE' EQ 'YES').G41
         MNOTE 3,'PAGE PARAMETER INCORRECT - ''NO'' ASSUMED'
         AGO   .G42
.G41     ANOP
&BLPAGE  SETB  (1)
.G42     ANOP
         AIF   ('&LOC' NE 'ANY').G71
&BLLOC   SETB  (1)
.G71     ANOP
         AIF   ('&POOL' EQ 'NO').G44
         AIF   ('&POOL' EQ 'YES').G43
         MNOTE 3,'POOL PARAMETER INCORRECT - ''NO'' ASSUMED'
         AGO   .G44
.G43     ANOP
&BLPOOL  SETB  (1)
.G44     ANOP
         AIF   ('&SVA' EQ 'NO').G46
         AIF   ('&SVA' EQ 'YES').G45
         MNOTE 3,'SVA PARAMETER INCORRECT - ''NO'' ASSUMED'
         AGO   .G46
.G45     ANOP
&BLSVA   SETB  (1)
.G46     ANOP
         AIF   (&BLPAGE OR &BLPOOL OR &BLSVA OR &BLLOC).G47
         SR    15,15              CLEAR INDICATORS
         AGO   .G48
.G47     ANOP
         CNOP  0,4
         BAL   15,*+8
         DC    H'0'    NEED A FULL WORD ALLOCATED
         DC    B'&BLLOC.0000000000&BLSVA&BLPOOL&BLPAGE' SET INDICATORS
         L     15,0(,15)
.G48     ANOP
         SVC   61                 ISSUE SVC FOR GETVIS
         AIF   (T'&ADDRESS EQ 'O').G6
         AIF   ('&ADDRESS'(1,1) EQ '(').G5
         AIF   (T'&ADDRESS NE 'N').G49
         MNOTE 3,'ADDRESS PARAMETER INVALID - IGNORED'
         MEXIT
.G49     ANOP
         ST    1,&ADDRESS         STORE RETURNED ADDRESS
         AGO   .G6
.G5      ANOP
         AIF   (T'&ADDRESS(1) NE 'N').G56
         AIF   ('&ADDRESS(1)' EQ '1').G6
         AIF   (&ADDRESS(1) NE 15).G56
         MNOTE 4,'RETURN CODE IN REGISTER 15 WILL BE DESTROYED'
.G56     ANOP
         LR    &ADDRESS(1),1      LOAD RETURNED ADDRESS FROM REGISTER 1
.G6      ANOP
         MEND
*
* Public domain ACCPT macro from Fran Hensler
*
         TITLE 'A C C P T   M A C R O'
         MACRO
&NAME    ACCPT &ANSWER,&LENGTH,&UPPER=YES,&EOB=,&SHORT=
         GBLA  &LEN@CH LENGTH OF LONGEST INPUT TO CONVERT TO UPPER CASE
         GBLA  &LEN@NO LENGTH OF LONGEST NUMERIC INPUT           FUTURE
         GBLB  &TYPER@A    SWITCH FOR TYPER GENERATION
         LCLA  &LEN      ACTUAL LENGTH OF INPUT AREA
         LCLB  &R1,&R2   SWITCHES INDICATING REGISTER NOTATION
         LCLC  &LABEL,&MASK
&TYPER@A SETB  1                   TELL TYPER THAT ACCPT MACRO USED.
&LABEL   SETC  '&NAME'
         AIF   (T'&ANSWER NE 'O').OK1
         MNOTE 1,'PARAMETER MISSING -- MACRO IGNORED.'
         MEXIT
.OK1     AIF   ('&ANSWER'(1,1) NE '(').OK2
         AIF   ('&ANSWER(1)' LT '2' OR '&ANSWER(1)' GT '12').REGERR
&R1      SETB  (1)            INDICATE REGISTER NOTATION FOR &ANSWER
         AIF   (T'&LENGTH NE 'O').OK2
         MNOTE 1,'LENGTH PARAMETER MISSING -- MACRO IGNORED.'
         MEXIT
.OK2     AIF   (T'&LENGTH EQ 'O').THREE
         AIF   ('&LENGTH'(1,1) NE '(').TWO
&R2      SETB  (1)            INDICATE REGISTER NOTATION FOR LENGTH
&LABEL   STH   &LENGTH(1),*+8 ..... STORE INPUT LENGTH.
&LABEL   SETC  ''
&LEN     SETA  0
         AGO   .FIVE
.TWO     ANOP
&LEN     SETA  &LENGTH
         AGO   .FIVE
.THREE   ANOP
&LEN     SETA  L'&ANSWER
.FIVE    ANOP
&LABEL   BAL   14,TYPERGET ..... MACRO FOR CONSOLE INPUT.      SRSC 1-5
         DC    AL2(&LEN) ..... LENGTH OF INPUT.
         AIF   (NOT &R1).SIX
         DC    S(000&ANSWER) ..... ADDRESS OF INPUT AREA.
         AGO   .SEVEN
.SIX     ANOP
         DC    S(&ANSWER) ..... ADDRESS OF INPUT AREA.
.SEVEN   AIF   ('&EOB' EQ '').NINE
         LTR   1,1 ..... WAS INPUT EOB?
         BZ    &EOB ..... BRANCH IF YES.
.NINE    AIF   ('&UPPER' EQ 'NO').ELEVEN
         AIF   ('&UPPER' NE 'YES').NINEA
&MASK    SETC  'TYPERBLK'
         AIF   (&LEN@CH GE &LEN).NINEB
&LEN@CH  SETA  &LEN
         AGO   .NINEB
.NINEA   ANOP
&MASK    SETC  '&UPPER'
.NINEB   AIF   (&R2).TEN
         AIF   (&R1).NINED
         AIF   (&LEN NE 1).NINEC
         OI    &ANSWER,B'01000000' ..... CONVERT TO UPPER CASE.
         AGO   .ELEVEN
.NINEC   OC    &ANSWER.(&LEN.),&MASK ..... CONVERT TO UPPER CASE.
         AGO   .ELEVEN
.NINED   AIF   (&LEN NE 1).NINEE
         OI    0&ANSWER,B'01000000' ..... CONVERT TO UPPER CASE.
         AGO   .ELEVEN
.NINEE   ANOP
         OC    0(&LEN.,&ANSWER(1).),&MASK ..... CONVERT TO UPPER CASE.
         SPACE 1
         MEXIT
.TEN     ANOP
         LR    0,1 ..... SAVE REGISTER 1.
.*       LA    14,&ANSWER(1) ..... GET ADDRESS OF INPUT.
.*  This LA changed to LR 01/09/97 -- I don't know how LA ever worked.
         LR    14,&ANSWER(1) ..... GET ADDRESS OF INPUT.
         OI    0(14),X'40' ..... CONVERT TO UPPER CASE.
         LA    14,1(14) ..... INCREMENT TO NEXT POSITION.
         BCT   1,*-8 ..... LOOP THRU INPUT.
         LR    1,0 ..... RESTORE REGISTER 1.
.ELEVEN  AIF   ('&SHORT' EQ '').END
         CH    1,0(14) ..... WAS INPUT SHORT?
         BL    &SHORT ..... BRANCH IF YES.
.END     SPACE 1
         MEXIT
.REGERR  MNOTE 1,'ONLY REGISTERS 2-12 MAY BE USED FOR PARAMETERS.'
         MEXIT
         MEND
*
* Public domain DSPLY macro from Fran Hensler
*
         TITLE 'D S P L Y   M A C R O'
         MACRO
&NAME    DSPLY &MESSAGE,&LENGTH,&UPON=SYSLOG
         GBLB  &TYPER@D,&TYPER@L   SWITCH FOR TYPER GENERATION
         LCLA  &LEN
         LCLB  &SW
         LCLC  &LABEL,&AMESS
&SW      SETB  (0)
&LABEL   SETC  '&NAME'
         AIF   (T'&MESSAGE EQ 'O').ERR1
         AIF   ('&MESSAGE'(1,1) NE '''').ONE
&LEN     SETA  K'&MESSAGE-2
&LABEL   LA    1,=CL&LEN&MESSAGE ..... GET ADDRESS OF OUTPUT AREA.
&LABEL   SETC  ''
&AMESS   SETC  '000(1)'
         AGO   .FOUR
.ONE     AIF   ('&MESSAGE'(1,1) NE '(').TWO
&AMESS   SETC  '000&MESSAGE'
         AIF   ('&MESSAGE(1)' LT '2' OR '&MESSAGE(1)' GT '12').REGERR
         AIF   (T'&LENGTH EQ 'O').MISS
         AGO   .TWOB
.TWO     ANOP
&AMESS   SETC  '&MESSAGE'
         AIF   (T'&LENGTH NE 'O').TWOA
&LEN     SETA  L'&MESSAGE
.TWOA    AIF   (T'&LENGTH EQ 'O').FOUR
.TWOB    ANOP
         AIF   ('&LENGTH'(1,1) NE '(').THREE
&LABEL   STH   &LENGTH(1),*+8 ..... STORE THE LENGTH.
&LABEL   SETC  ''
&LEN     SETA  0
         AGO   .FOUR
.THREE   ANOP
&SW      SETB  (1)
.FOUR    ANOP
         AIF   ('&UPON' EQ 'SYSLST').SYSLST
&LABEL   BAL   14,TYPERPUT ..... MACRO FOR CONSOLE DISPLAY     SRSC 1-3
&TYPER@D SETB  (1)                 TELL TYPER THAT SYSLOG BEING USED.
         AGO   .FOUR1
.SYSLST  ANOP
&LABEL   BAL   14,TYPERLST ..... MACRO FOR PRINTER DISPLAY     SRSC 1-4
&TYPER@L SETB  (1)                 TELL TYPER THAT SYSLST BEING USED.
.FOUR1   AIF   (&SW).FOURA
         DC    AL2(&LEN)  ..... LENGTH OF OUTPUT AREA.
         AGO   .FIVE
.FOURA   ANOP
         DC    AL2(&LENGTH) ..... LENGTH OF OUTPUT AREA.
.FIVE    ANOP
         DC    S(&AMESS) ..... ADDRESS OF OUTPUT AREA.
         SPACE 1
         MEXIT
.ERR1    MNOTE 1,'PARAMETER MISSING -- MACRO IGNORED.'
         MEXIT
.MISS    MNOTE 1,'LENGTH PARAMETER MISSING -- MACRO IGNORED.'
         MEXIT
.REGERR  MNOTE 1,'ONLY REGISTERS 2-12 MAY BE USED FOR PARAMETERS.'
         MEXIT
         MEND
*
* Public domain TYPER macro from Fran Hensler
*
         TITLE 'T Y P E R   M A C R O'
         MACRO
         TYPER &RELO=YES
         GBLA  &LEN@CH LENGTH OF LONGEST INPUT TO CONVERT TO UPPER CASE
         GBLA  &LEN@NO LENGTH OF LONGEST NUMERIC INPUT           FUTURE
         GBLB  &TYPER@A,&TYPER@D,&TYPER@L    GENERATION SWITCHES
         DC    0H'0' * T Y P E R * DSPLY & ACCPT I/O MODULE    SRSC 1-5
         AIF   (NOT &TYPER@L).NOLIST    CHECK IF SYSLST
TYPERLST EQU   *                   WRITE A MESSAGE ON SYSLST        1-4
         AIF   (NOT &TYPER@D AND NOT &TYPER@A).LSTONLY
         MVI   TYPERCCB+7,3        CHANGE CCB LOGICAL UNIT TO SYSLST
.NOLIST  AIF   (NOT &TYPER@D AND NOT &TYPER@L).NODSPLY
TYPERPUT EQU   *                   WRITE A MESSAGE ON THE CONSOLE   1-3
.LSTONLY MVC   *+8(2),2(14)        PUT DATA ADDRESS IN NEXT INSTRUCTION
         LA    1,*-*               GET DATA ADDRESS IN REGISTER 1.
         STCM  1,7,TYPERCCW+1      PUT DATA ADDRESS IN CCW.
         AIF   (NOT &TYPER@A).NOACCPT
         MVI   TYPERCCW,9          SET COMMAND CODE TO WRITE & SPACE 1
         B     TYPERXCP            BRANCH TO EXECUTE CHANNEL PROGRAM.
.NODSPLY AIF   (&TYPER@A).ACCPT
         MNOTE 1,'DSPLY AND/OR ACCPT MACROS MUST PRECEDE TYPER, NOGEN'
         MEXIT
.ACCPT   ANOP
TYPERGET EQU   *                   ACCEPT INPUT FROM CONSOLE        1-3
         MVC   *+8(2),2(14)        PUT DATA ADDRESS IN NEXT INSTRUCTION
         LA    1,*-*               GET DATA ADDRESS IN REGISTER 1.
         LR    0,14                SAVE REGISTER 14 IN 0.
         MVI   0(1),64             PUT BLANK IN 1ST POSITION OF INPUT.
         LH    14,0(14)            GET DATA LENGTH IN REG 14.
         BCTR  14,0                REDUCE LENGTH BY 1.
         LTR   14,14               IS REGISTER 14 ZERO.
         BZ    *+10                BRANCH AROUND NEXT 2 INSTRUCTIONS.
         BCTR  14,0
         EX    14,TYPERMVC         SET INPUT AREA TO BLANKS.
         LR    14,0                RESTORE REGISTER 14 FROM 0.
         STCM  1,7,TYPERCCW+1      PUT DATA ADDRESS IN CCW.
         MVI   TYPERCCW,10         SET COMMAND CODE TO READ
TYPERXCP EQU   *
.NOACCPT MVC   TYPERCCW+7(1),1(14) PUT I/O AREA LENGTH IN CCW.
         AIF   ('&RELO' EQ 'NO').NORELO
         LA    1,TYPERCCW          MAKE CHANNEL PROGRAM
         STCM  1,7,TYPERCCB+9       RELOCATABLE.
.NORELO  LA    1,TYPERCCB          GET ADDRESS OF CHANNEL PROGRAM.
         SVC   0                   EXECUTE CHANNEL PROGRAM.
         TM    2(1),X'80'          WAIT
         BO    *+6                  UNTIL
         SVC   7                     COMPLETED.
         AIF   (NOT &TYPER@A).TWO
         TM    4(1),1              WAS INPUT MESSAGE CANCELLED.
         BNZ   TYPERGET            YES -- TRY AGAIN.
         LH    1,TYPERCCW+6        GET ACTUAL RECORD LENGTH.
         SH    1,TYPERCCB  CCW COUNT - RESIDUAL COUNT = RECORD LENGTH.
.TWO  AIF  ((NOT &TYPER@L) OR (NOT &TYPER@D AND NOT &TYPER@A)).THREE
         MVI   TYPERCCB+7,4        CHANGE LOGICAL UNIT TO SYSLOG.
.THREE   B     4(14)               RETURN TO CALLER + 4.
TYPERCCW CCW   9,*-*,0,*-*
         AIF   (NOT &TYPER@D AND NOT &TYPER@A).LSTCCB
TYPERCCB CCB   SYSLOG,TYPERCCW
         AGO   .CCBDONE
.LSTCCB  ANOP
TYPERCCB CCB   SYSLST,TYPERCCW
.CCBDONE AIF   (NOT &TYPER@A).FOUR
         AIF   (&LEN@CH LE 1).NOBLK
TYPERBLK DC    &LEN@CH.X'40'       BLANKS TO CONVERT TO UPPER CASE.
.NOBLK   AIF   (&LEN@NO EQ 0).NONUM                              FUTURE
TYPERNUM DC    &LEN@NO.Z'0'        ZEROES TO TEST NUMERIC INPUT. FUTURE
.NONUM   ANOP
TYPERMVC MVC   1(*-*,1),0(1)       SUBJECT OF EX INSTRUCTION.
.FOUR    SPACE 1
         MEXIT
         MEND
*
* MAPACCT - map ACCTABLE - from Fran Hensler
*
         MACRO 
         MAPACCT
* SRSC - MAP OF JOB ACCOUNTING PARTITION TABLE. NAMES SAME AS $$A$SUP1
*        ADDRESSABILITY FROM LABEL 'JAPART' IN MAPCOMR MACRO.
ACCTABLE DSECT                    PARTITION ACCOUNTING TABLE
ACCTWK1  DS    F                  WORK AREAS
ACCTWK2  DS    F
ACCTSVPT DS    F                       SAVE AREA FOR JOB CARD PTR
ACCTPART DS    X                       PARTITION SWITCH KEY
ACCTRES2 DS    X                       RESERVED
ACCTLEN  DS    H                  LENGTH OF SIO PART OF TABLE.
ACCTLOAD DS    3H                      INST TO SET LABEL AREA
ACCTRES3 DS    H                       RESERVED
ACCTLADD DS    A                       ADDR OF LABEL AREA
ACCTCPUT DS    F                       PARTITION CPU TIME COUNTER
ACCTOVHT DS    F                       OVERHEAD CPU COUNTER
ACCTBNDT DS    F                       WAIT TIME COUNTER
ACCTSVJN DS    CL8                     JOB NAME SAVE AREA
*        REG 15 HAS ADDRESS OF FOLLOWING LABEL WHEN $JOBACCT CALLED.
ACCTJBNM DS    CL8                START OF USERS SECTION OF TABLE.
ACCTUSRS DS    CL16               USERS ACCOUNT INFORMATION
ACCTPTID DS    CL2                PARTITION ID
ACCTCNCL DS    XL1                CANCEL CODE FOR JOB STEP
ACCTYPER DS    XL1                TYPE OF ACCOUNTING RECORD
ACCTDATE DS    CL8                DATE OF JOB
ACCTSTRT DS    F                  JOB START TIME
ACCTSTOP DS    F                  JOB STOP TIME
ACCTRESV DS    F                  RESERVED
ACCTEXEC DS    CL8                JOB STEP PHASE NAME
ACCTHICR DS    F                  JOB STEP HI-CORE ADDRESS
ACCTIMES DS    3F                 EXECUTION TIME BREAKDOWN FIELDS
ACCTSIOS DS    X                  START OF SIO TABLE
         SPACE 2
*        FOLLOWING IS MAP OF 16 BYTE USER AREA (ACCTUSRS).
         SPACE 1
         ORG   ACCTUSRS
ACCTSSN  DS    F'999999999'        USER SOCIAL SECURITY NUMBER.
ACCTTIME DS    H'03'               ESTIMATED RUN TIME (MINUTES).
ACCTCOST DS    PL3'21200'          COST CENTER NUMBER.
ACCTTYPE DS    CL1'P'              T=TEST P=PROD S=STUDENT
ACCTRJE  DS    CL5'WC105'          RJE TERMINAL ID, BLANK IF LOCAL JOB.
         DS    X                   RESERVED FOR FUTURE USE.
         MEND
undivert(pdpprlg.mac)undivert(pdpepil.mac)         END
/*
// DLBL IJSYSIN,'WORK.FILE',0,SD
// EXTENT SYSIPT,WORK01
CLOSE SYSPCH,PUNCH                                    
ASSGN SYSIPT,DISK,VOL=WORK01,SHR
*
* Now do the copy libs
*
// EXEC MAINT                                         
CLOSE SYSIPT,READER
// EXEC MAINT
 CATALS A.PDPTOP
 BKEND
undivert(pdptop.mac) BKEND
/*
*
* Now assemble the main routine
*
* // OPTION LINK
// OPTION CATAL
 PHASE PDPTEST,*
// EXEC ASSEMBLY
undivert(vsestart.asm)/*
*
* Now assemble the subroutines
*
* // OPTION LINK
*  // OPTION CATAL
// EXEC ASSEMBLY
undivert(start.s)/*
// EXEC ASSEMBLY
undivert(stdio.s)/*
// EXEC ASSEMBLY
undivert(stdlib.s)/*
// EXEC ASSEMBLY
undivert(ctype.s)/*
// EXEC ASSEMBLY
undivert(string.s)/*
// EXEC ASSEMBLY
undivert(time.s)/*
// EXEC ASSEMBLY
undivert(errno.s)/*
// EXEC ASSEMBLY
undivert(assert.s)/*
// EXEC ASSEMBLY
undivert(locale.s)/*
// EXEC ASSEMBLY
undivert(math.s)/*
// EXEC ASSEMBLY
undivert(setjmp.s)/*
// EXEC ASSEMBLY
undivert(signal.s)/*
// EXEC ASSEMBLY
undivert(__memmgr.s)/*
// EXEC ASSEMBLY
undivert(pdptest.s)/*
// EXEC ASSEMBLY
undivert(vsesupa.asm)/*
*
* Now link the whole app
*
// EXEC LNKEDT
 ACTION NOREL
/*
*
* Now run the app
*
* Inserting these statements appropriately can provide parameters
* from SYSINPT instead. Need to use '?' for SYSPARM in that case.
*
* Oh what fun it is to ride in a one-horse open sleigh, hey!
* // UPSI 10000000
// OPTION DUMP
// OPTION SYSPARM='aBc DeF'
// ASSGN SYS010,DISK,VOL=WORK01,SHR
// DLBL SDIN,'TEST.F80'
// EXTENT SYS010,WORK01
// EXEC PDPTEST,SIZE=AUTO,PARM='Hi there'
/*
/&
* $$ EOJ
