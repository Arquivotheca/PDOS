* $$ JOB JNM=TRY31BIT
* $$ LST LST=SYSLST,CLASS=A
// JOB TRY31BIT
*
* Get the direct macros in
* Note that most of these need to be moved to VSE/380
*
*
// OPTION EDECK,NODECK  
// DLBL IJSYSPH,'WORK.FILE',0,SD
// EXTENT SYSPCH,WORK01,1,0,3,6
ASSGN SYSPCH,DISK,VOL=WORK01,SHR
// EXEC ASSEMBLY
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
