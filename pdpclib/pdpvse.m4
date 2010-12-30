* $$ JOB JNM=VSEJOB
* $$ LST LST=SYSLST,CLASS=A
// JOB VSEJOB
*
*
*
* Standard C parms
*
// ASSGN SYS000,DISK,VOL=WORK01,SHR
// ASSGN SYS005,SYSLST
// ASSGN SYS007,SYSPCH
// OPTION DUMP
*
*
*
* Get the direct macros in
*
// OPTION EDECK,NODECK  
// DLBL IJSYSPH,'WORK.FILE',0,SD
// EXTENT SYSPCH,,,,3,6
ASSGN SYSPCH,DISK,VOL=WORK01,SHR
// EXEC ASSEMBLY
undivert(pdpprlg.mac)undivert(pdpepil.mac)         END
/*
CLOSE SYSPCH,PUNCH
// OPTION NOEDECK
*
*
// DLBL IJSYSIN,'WORK.FILE',0,SD
ASSGN SYSIPT,DISK,VOL=WORK01,SHR
// EXEC MAINT                                         
CLOSE SYSIPT,READER
*
*
*
*
* Now do the copy libs
*
// EXEC MAINT
 CATALS A.PDPTOP
 BKEND
undivert(pdptop.mac) BKEND
/*
*
*
* Now assemble the main routine
*
// OPTION CATAL
 PHASE PDPTEST,S+80
// EXEC ASSEMBLY
undivert(vsestart.asm)/*
*
* Now assemble the subroutines
*
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
*
*
*
* Now run the app
*
// EXEC PDPTEST,SIZE=AUTO,PARM='one two Three'
*
*
*
/&
* $$ EOJ
