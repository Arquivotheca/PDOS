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
