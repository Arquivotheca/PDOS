* $$ JOB JNM=TRY31BIT
* $$ LST LST=SYSLST,CLASS=A
// JOB TRY31BIT
*
* Get the direct macros in
*
*
// OPTION EDECK,NODECK  
// DLBL IJSYSPH,'WORK.FILE',0,SD
// EXTENT SYSPCH,WORK01,1,0,3,6
ASSGN SYSPCH,DISK,VOL=WORK01,SHR
// EXEC ASSEMBLY
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
* Allow room for a standard-label tape, just in case one is used
// LBLTYP TAPE
// EXEC LNKEDT
*
* Now run the app
*
// ASSGN SYS005,SYSLST
// OPTION DUMP
// OPTION SYSPARM='aBc DeF'
// EXEC PDPTEST,SIZE=999K
/*
/&
* $$ EOJ
