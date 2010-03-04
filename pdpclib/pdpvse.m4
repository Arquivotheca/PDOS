* $$ JOB JNM=TRY31BIT,CLASS=0,USER='ANONYMOUS'
* $$ LST LST=SYSLST,CLASS=A,DISP=D,JSEP=1
// JOB TRY31BIT
*
* Get the direct macros in first
*
*
// OPTION EDECK,NODECK  
// DLBL IJSYSPH,'WORK.FILE',0,SD
// EXTENT SYSPCH,WORK01,1,0,3,6
ASSGN SYSPCH,DISK,VOL=WORK01,SHR
// EXEC ASSEMBLY
         MACRO
&N       AMODE
         MEND
         MACRO
&N       RMODE
         MEND
         MACRO
&LABEL   BSM   &R1,&R2
         DS    0H
&LABEL.  DC    0XL2'00',X'0B',AL.4(&R1.,&R2.)
         MEND
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
         MACRO
&NAME GETVIS &ERROR,&ADDRESS=,&LENGTH=,&PAGE=NO,&POOL=NO,&SVA=NO
 LCLB &BLPAGE,&BLPOOL,&BLSVA
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
         AIF   (&BLPAGE OR &BLPOOL OR &BLSVA).G47
         SR    15,15              CLEAR INDICATORS
         AGO   .G48
.G47     ANOP
         LA    15,B'00000&BLSVA&BLPOOL&BLPAGE' SET INDICATORS
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
// OPTION DUMP
// EXEC PDPTEST,SIZE=999K
/*
/&
* $$ EOJ
