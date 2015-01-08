MVSSUPA  TITLE 'M V S S U P A  ***  MVS VERSION OF PDP CLIB SUPPORT'
***********************************************************************
*                                                                     *
*  This program written by Paul Edwards.                              *
*  Released to the public domain                                      *
*                                                                     *
*  Extensively modified by others                                     *
*                                                                     *
***********************************************************************
*                                                                     *
*  MVSSUPA - Support routines for PDPCLIB under MVS                   *
*    Additional macros in (EDWARDS.)PDPCLIB.MACLIB                    *
*  It is currently coded for GCC, but C/370 functionality is          *
*  still there, it's just not being tested after each change.         *
*                                                                     *
***********************************************************************
*                                                                     *
* Note that some of the functionality in here has not been exercised  *
* to any great extent, since it is dependent on whether the C code    *
* invokes it or not.                                                  *
*                                                                     *
* Note that this code issues WTOs. It should be changed to just       *
* set a return code and exit gracefully instead.                      *
*                                                                     *
***********************************************************************
*   Changes by Gerhard Postpischil:
*     EQU * for entry points deleted (placed labels on SAVE) to avoid
*       0C6 abends when EQU follows a LTORG
*     Fixed 0C4 abend in RECFM=Vxxx processing; fixed PUT length error.
*     Deleted unnecessary and duplicated instructions
*     Added @@SYSTEM and @@DYNAL routines                2008-06-10
*     Added @@IDCAMS non-reentrant, non-refreshable      2008-06-17
*     Modified I/O for BSAM, EXCP, and terminal I/O
*     Added checks to AOPEN to support unlike PDS BLDL   2014-03-03
*     Caller may use @@ADCBA any time to get current DCB attributes.
*     Added support for unlike PDS concatenation; requires member.
*     Fixed problems with sequential unlike concatenation:
*       When next DD is entered, AREAD returns with R15=4, no data.
*       Use @@ADCBA to get attributes for next DD.
*     Largest blocksize will be used for buffer.         2014-07-24
*     The program now supports reading the VTOC of a disk pack;
*     use @@AOPEN, @@ACLOSE, @@AREAD normally for a sequential data
*       set with record length 140 (44 key, 96 data). The DD card:
*       //ddname DD DISP=OLD,DSN=FORMAT4.DSCB,UNIT=SYSALLDA,
*       //           VOL=SER=serial                      2014-08-01
*
***********************************************************************
*
*   To facilitate cross-assembly (S390 on S370/380 system), some
*   OS/390 & zOS macros replaced. Affected old statements are flagged
*   *COMP*                                               2015-01-06
*
***********************************************************************
*
*
* Internal macros:
*
*
         MACRO ,             PATTERN FOR @@DYNAL'S DYNAMIC WORK AREA
&NM      DYNPAT &P=MISSING-PFX
.*   NOTE THAT EXTRA FIELDS ARE DEFINED FOR FUTURE EXPANSION
.*
&NM      DS    0D            ALLOCATION FIELDS
&P.ARBP  DC    0F'0',A(X'80000000'+&P.ARB) RB POINTER
&P.ARB   DC    0F'0',AL1(20,S99VRBAL,0,0)
         DC    A(0,&P.ATXTP,0,0)       SVC 99 REQUEST BLOCK
&P.ATXTP DC    10A(0)
&P.AXVOL DC    Y(DALVLSER,1,6)
&P.AVOL  DC    CL6' '
&P.AXDSN DC    Y(DALDSNAM,1,44)
&P.ADSN  DC    CL44' '
&P.AXMEM DC    Y(DALMEMBR,1,8)
&P.AMEM  DC    CL8' '
&P.AXDSP DC    Y(DALSTATS,1,1)
&P.ADSP  DC    X'08'         DISP=SHR
&P.AXFRE DC    Y(DALCLOSE,0)   FREE=CLOSE
&P.AXDDN DC    Y(DALDDNAM,1,8)    DALDDNAM OR DALRTDDN
&P.ADDN  DC    CL8' '        SUPPLIED OR RETURNED DDNAME
&P.ALEN  EQU   *-&P.ARBP       LENGTH OF REQUEST BLOCK
         SPACE 1
&P.URBP  DC    0F'0',A(X'80000000'+&P.URB) RB POINTER
&P.URB   DC    0F'0',AL1(20,S99VRBUN,0,0)
         DC    A(0,&P.UTXTP,0,0)       SVC 99 REQUEST BLOCK
&P.UTXTP DC    A(X'80000000'+&P.UXDDN)
&P.UXDDN DC    Y(DUNDDNAM,1,8)
&P.UDDN  DC    CL8' '        RETURNED DDNAME
&P.ULEN  EQU   *-&P.URBP       LENGTH OF REQUEST BLOCK
&P.DYNLN EQU   *-&P.ARBP     LENGTH OF ALL DATA
         MEND  ,
*
*
*
         MACRO ,
&NM      FIXWRITE ,
&NM      L     R15,=A(TRUNCOUT)
         BALR  R14,R15       TRUNCATE CURRENT WRITE BLOCK
         MEND  ,
*
*
*
         MACRO ,
&NM      OSUBHEAD ,
         PUSH  USING
         DROP  ,
         USING WORKAREA,R13
         USING ZDCBAREA,R10
&NM      STM   R10,R15,SAVOSUB    Save registers                GP14233
         BALR  R12,0
         USING *,R12
         MEND  ,
         SPACE 1
*
*
*
         MACRO ,
&NM      OSUBRET &ROUTE=
         LCLC  &T
&T       SETC  '&NM'
         AIF   (T'&ROUTE EQ 'O').BACK
         AIF   ('&ROUTE' EQ '(14)' OR '&ROUTE' EQ '(R14)').ROUT14
&T       LA    R14,=A(&ROUTE)     Return point in AOPEN
         AGO   .BACK
&T       SETC  ''                 Set label used
.ROUT14  ANOP  ,
&T       ST    R14,SAVOSUB+4*4    Update R14
&T       SETC  ''                 Set label used
.BACK    ANOP  ,                                                GP14233
&T       LM    R10,R15,SAVOSUB    Load registers                GP14233
         BR    R14                Return to caller              GP14233
         MEND  ,
         SPACE 1
*
*
*
         MACRO ,
&NM      OPENCALL &WHOM
&NM      L     R15,=A(&WHOM)      Extension routine             GP14233
         BALR  R14,R15            Invoke it                     GP14233
         MEXIT ,
         MEND  ,
         SPACE 1
         MACRO ,
&NM      OBRAN &WHERE,&OP=B
&NM      L     R14,=A(&WHERE)     Return point                  GP14233
         &OP   VECTOR             Branch to alternate return    GP14233
         MEND  ,
         SPACE 1
*
*
*
         SPACE 1
         COPY  MVSMACS
         COPY  PDPTOP
         SPACE 1
* For S/390 we need to deliberately request LOC=BELOW storage
* in some places.
* For S/380 we need to deliberately request LOC=ANY storage.
* For all other environments, just let it naturally default
* to LOC=RES
*
MVSSUPA  CSECT ,
         PRINT GEN
         YREGS ,
         SPACE 1
*-----------------------ASSEMBLY OPTIONS------------------------------*
SUBPOOL  EQU   0                                                      *
*---------------------------------------------------------------------*
         SPACE 1
*                                                                     *
*   Note: Variable @@BUGF controls various debugging options, and is  *
*   externally accessible. Low order 1 bit requests SNAPs of failed   *
*   OPEN information and bad records.                                 *
         ENTRY @@BUGF                                           GP14251
*                                                                     *
*---------------------------------------------------------------------*
*                                                                     *
* Start of functions                                                  *
*                                                                     *
*                                                                     *
***********************************************************************
*                                                                     *
*  AOPEN - Open a data set                                            *
*                                                                     *
***********************************************************************
*                                                                     *
*  Parameters are:                                                    *
*1 DDNAME - space-padded, 8 character DDNAME to be opened             *
*                                                                     *
*2 MODE =  0 INPUT  1 OUTPUT  2 UPDAT   3 APPEND      Record mode     *
*  MODE =  4 INOUT  5 OUTIN     (6-7 reserved)                        *
*  MODE = 8/9 Use EXCP for tape, BSAM otherwise (or 32<=JFCPNCP<=65)  *
*  MODE + 10 = Use BLOCK mode (valid hex 10-15)                       *
*  MODE = 80 = GETLINE, 81 = PUTLINE (other bits ignored)             *
*    N.B.: see comments under Return value                            *
*                                                                     *
*3 RECFM - 0 = F, 1 = V, 2 = U. Default/preference set by caller;     *
*                               actual value returned from open.      *
*                                                                     *
*4 LRECL   - Default/preference set by caller; OPEN value returned.   *
*                                                                     *
*5 BLKSIZE - Default/preference set by caller; OPEN value returned.   *
*                                                                     *
* August 2009 revision - caller will pass preferred RECFM (coded 0-2) *
*    LRECL, and BLKSIZE values. DCB OPEN exit OCDCBEX will use these  *
*    defaults when not specified on JCL or via DSCB merge.            *
*                                                                     *
*6 ZBUFF2 - pointer to an area that may be written to (size is LRECL) *
*                                                                     *
*7 MEMBER - *pointer* to space-padded, 8 character member name.       *
*    A member name beginning with blank or hex zero is ignored.       *
*    If pointer is 0 (NULL), no member is requested                   *
*    For a DD card specifying a PDS with a member name, this parameter*
*    will replace the JCL member, unless the DD is concatenated, then *
*    all DDs are treated as sequential and a member operand will be   *
*    an error.                                                        *
*                                                                     *
*                                                                     *
*  Return value: In R15                                               *
*  An internal "handle" that allows the assembler routines to         *
*  keep track of what's what, when READ etc are subsequently          *
*  called.                                                            *
*                                                                     *
*                                                                     *
*  Return value: PARM2 MODE:                                          *
*    Byte 0 - major device type (defined by UCBTBYT3; JES sets 01)    *
*    Byte 1 - true RECFM as used in the DCB                           *
*    Byte 2 - processing mode (see IOSFLAGS)                          *
*    Byte 3 - modified user's MODE                                    *
*                                                                     *
*  All passed parameters are subject to overrides based on device     *
*  capabilities and capacities, e.g., blocking may be turned off.     *
*                                                                     *
*                                                                     *
*  Note - more documentation for this and other I/O functions can     *
*  be found halfway through the stdio.c file in (EDWARDS.)PDPCLIB.    *
*                                                                     *
* Here are some of the errors reported:                               *
*                                                                     *
*  Input  OPEN (SVC) failed; return code is: -37                      *
*  Output OPEN (SVC) failed; return code is: -39                      *
*    Also used for VSAM failure in OPEN, SHOWCB, or TESTCB            *
*                                                                     *
* FIND input member return codes are:                                 *
* Original, before the return and reason codes had                    *
* negative translations added refer to copyrighted:                   *
* DFSMS Macro Instructions for Data Sets.                             *
* RC = 0 Member was found.                                            *
*                                                                     *
*     The 1nnn group has not been implemented.                        *
* RC = -1024 Member not found. (replaced by 2068)                     *
* RC = -1028 RACF allows PDSE EXECUTE, not PDSE READ.                 *
* RC = -1032 PDSE share not available.                                *
* RC = -1036 PDSE is OPENed output to a different member.             *
*---------------------------------------------------------------------*
*   New OPEN validity checking added; return codes are:               *
* RC = -2004 DDname starts with blank or null                         *
* RC = -2008 DDname not found                                         *
* RC = -2012 Error in system control block (JFCB, JSCB, PSCB)         *
* RC = -2016 Error reading DSCB1                                      *
* RC = -2020 Invalid TIOT entry                                       *
* RC = -2024 Invalid or unsupported DSORG                             *
* RC = -2028 Invalid DCB parameters                                   *
* RC = -2032 Invalid unit type (Graphics, Communications...)          *
* RC = -2036 Invalid concatenation (not input; mixed sequential & PDS)*
* RC = -2040 Invalid MODE request for DD                              *
* RC = -2044 PDS has no directory blocks                              *
* RC = -2048 Directory I/O error.                                     *
* RC = -2052 Out of virtual storage.                                  *
* RC = -2056 Invalid DEB or DEB not on TCB or TCBs DEB chain.         *
* RC = -2060 PDSE I/O error flushing system buffers.                  *
* RC = -2064 Invalid FIND or BLDL.                                    *
* RC = -2068 Member not found                                         *
* RC = -2072 Member not allowed                                       *
* RC = -30nn VSAM OPEN failed with ACBERF=nn                          *
*                                                                     *
***********************************************************************
         PUSH  USING
@@AOPEN  FUNHEAD SAVE=(WORKAREA,OPENLEN,SUBPOOL)
         SR    R10,R10            Indicate no ZDCB area gotten  GP14205
         LR    R11,R1             KEEP R11 FOR PARAMETERS
         USING PARMSECT,R11       MAKE IT EASIER TO READ
         LDADD R3,PARM1           R3 POINTS TO DDNAME           GP14251
         PUSH  USING                                            GP14205
***********************************************************************
**                                                                   **
**  Code added to support unlike concatenation for both sequential   **
**  and partitioned access (one member from FB or VB library DDs).   **
**  Determines maximum buffer size need for both cases (except tape) **
**                                                                   **
**  Added validity checking and error codes.                         **
**                                                                   **
**  Does not use R3, R11-R13                                         **
**                                                                   **
***********************************************************************
         MVI   OPERF,ORFBADNM     PRESET FOR BAD DD NAME        GP14205
         CLI   0(R3),C' '         VALID NAME ?                  GP14205
         BNH   OPSERR               NO; REJECT IT               GP14205
         MVI   OPERF,ORFNODD      PRESET FOR MISSING DD NAME    GP14205
         LR    R8,R3              COPY DDNAME POINTER TO SCRATCH REG.
         SAM31 ,                                                GP14205
         LA    R4,DDWATTR-DDASIZE POINTER TO DD ATTRIBUTES      GP14205
         USING DDATTRIB,R4        DECLARE TABLE                 GP14205
         L     R14,PSATOLD-PSA    GET MY TCB                    GP14205
         L     R9,TCBTIO-TCB(,R14) GET TIOT                     GP14205
         USING TIOT1,R9           DECLARE IT                    GP14205
         LA    R0,TIOENTRY-TIOT1  INCREMENT TO FIRST ENTRY      GP14205
*---------------------------------------------------------------------*
*   LOOK FOR FIRST (OR ONLY) DD                                       *
*---------------------------------------------------------------------*
DDCFDD1  AR    R9,R0              NEXT ENTRY                    GP14205
         MVI   OPERF,ORFNODD      PRESET FOR NO TIOT ENTRY      GP14205
         USING TIOENTRY,R9        DECLARE IT                    GP14205
         ICM   R0,1,TIOELNGH      GET ENTRY LENGTH              GP14205
         BZ    DDCTDONE             TOO BAD                     GP14205
         TM    TIOESTTA,TIOSLTYP  SCRATCHED ENTRY?              GP14205
         BNZ   DDCFDD1              YES; IGNORE                 GP14205
         CLC   TIOEDDNM,0(R8)     MATCHES USER REQUEST?         GP14205
         BNE   DDCFDD1              NO; TRY AGAIN               GP14205
         SR    R7,R7                                            GP14205
         ICM   R7,7,TIOEFSRT      LOAD UCB ADDRESS (COULD BE ZERO)
         USING UCBOB,R7                                         GP14205
         MVI   OPERF,ORFBATIO     SET FOR INVALID TIOT          GP14205
         CLI   TIOELNGH,20        SINGLE UCB ?                  GP14205
         BL    OPSERR               NOT EVEN                    GP14205
*---------------------------------------------------------------------*
* EXAMINE ONE DD ENTRY, AND SET FLAGS AND BUFFER SIZE HIGH-WATER MARK *
*---------------------------------------------------------------------*
         SPACE 1                                                GP14205
DDCHECK  MVI   OPERF,ORFNOJFC     PRESET FOR BAD JFCB           GP14205
         ICM   R1,7,TIOEJFCB      GET JFCB ADDRESS OR TOKEN     GP15006
         BZ    OPSERR               NO JFCB ?                   GP15006
         L     R15,=A(LOOKSWA)    GET TOKEN CONVERSION          GP15006
         BALR  R14,R15            INVOKE IT                     GP15006
         LTR   R6,R15             LOAD AND TEST ADDRESS         GP15006
         BNP   OPSERR               NO JFCB ?                   GP15006
*COMP*   AIF   ('&ZSYS' NE 'S390').MVSJFCB                      GP14205
*COMP*   XC    DDWSWA(DDWSWAL),DDWSWA  CLEAR SWA LIST FORM      GP14205
*COMP*   LA    R1,DDWSVA          ADDRESS OF JFCB TOKEN         GP14205
*COMP*   ST    R1,DDWEPA                                        GP14205
*COMP*   MVC   DDWSVA+4(3),TIOEJFCB    JFCB TOKEN               GP14205
*COMP*   SWAREQ FCODE=RL,EPA=DDWEPA,MF=(E,DDWSWA),UNAUTH=YES    GP14205
*COMP*   BXH   R15,R15,OPSERR                                   GP14205
*COMP*   ICM   R6,15,DDWSVA       LOAD JFCB ADDRESS             GP14205
*COMP*   BZ    OPSERR               NO; SERIOUS PROBLEM         GP14205
*COMP*   AGO   .COMJFCB                                         GP14205
*MVSJFCB SR    R6,R6              FOR AM31                      GP14205
*COMP*   ICM   R6,7,TIOEJFCB      SHOULD NEVER BE ZERO          GP14205
*COMP*   BZ    OPSERR               NO; SERIOUS PROBLEM         GP14205
*COMP*   LA    R6,16(,R6)         SKIP QUEUE HEADER             GP14205
.COMJFCB MVC   MYJFCB(JFCBLGTH),0(R6)   MOVE TO MY STORAGE      GP14205
         OI    DDWFLAG2,CWFDD     DD FOUND                      GP14205
         MVC   DDADSORG,JFCDSORG  SAVE                          GP14205
         MVC   DDARECFM,JFCRECFM    DCB                         GP14205
         MVC   DDALRECL,JFCLRECL      ATTRIBUTES                GP14205
         MVC   DDABLKSI,JFCBUFSI                                GP14205
         CLC   JFCDSORG,ZEROES    ANY DSORG HERE?               GP14205
         BE    DDCNOORG             NO                          GP14205
         TM    JFCDSRG2,JFCORGAM  VSAM ?                        GP14233
         BNZ   DDCNOORG             YES; SPECIAL HANDLING       GP14233
         CLI   JFCDSRG2,0         ANYTHING UNWANTED?            GP14205
         BNE   BADDSORG             YES; USE IT                 GP14205
         TM    JFCDSRG1,254-JFCORGPS-JFCORGPO  UNSUPPORTED ?    GP14205
         BNZ   BADDSORG             YES; FAIL                   GP14205
DDCNOORG SR    R5,R5                                            GP14205
         ICM   R5,3,JFCBUFSI      ANY BLOCK/BUFFER SIZE ?       GP14205
         C     R5,DDWBLOCK        COMPARE TO PRIOR VALUE        GP14205
         BNH   DDCNJBLK             NOT LARGER                  GP14205
         ST    R5,DDWBLOCK        SAVE FOR RETURN               GP14205
DDCNJBLK LTR   R7,R7              IS THERE A UCB (OR TOKEN)?    GP14205
         BZ    DDCSEQ               NO; MUST NOT BE A PDS       GP14205
         MVI   OPERF,ORFBATY3     PRESET UNSUPPORTED DEVTYPE    GP14205
         CLI   UCBTBYT3,UCB3DACC  DASD ?                        GP14205
         BNE   DDCNRPS              NO                          GP14205
         MVC   DDAATTR,UCBTBYT2   COPY ATTRIBUTES               GP14205
         NI    DDAATTR,UCBRPS     KEEP ONLY RPS                 GP14205
DDCNRPS  TM    UCBTBYT3,255-(UCB3DACC+UCB3TAPE+UCB3UREC)        GP14205
         BNZ   OPSERR               UNSUPPORTED DEVICE TYPE     GP14205
         CLI   UCBTBYT3,UCB3DACC  DASD VOLUME ?                 GP14205
         BNE   DDCSEQ               NO; NOT PDS                 GP14205
         CLC   =C'FORMAT4.DSCB ',JFCBDSNM   VTOC READ?          GP14213
         BNE   NOTVTOC                                          GP14213
         LA    R0,ORFBDMOD        PRESET FOR BAD MODE           GP14251
         LDVAL R14,PARM2          GET THE MODE                  GP14251
         TM    3(R14),X'07'       ANYTHING OTHER THAN INPUT?    GP14251
         BNZ   OPRERR             VTOC WRITE NOT SUPPORTED      GP14251
         MVI   JFCBDSNM,X'04'     MAKE VTOC 'NAME'              GP14213
         MVC   JFCBDSNM+1(L'JFCBDSNM-1),JFCBDSNM   44X'04'      GP14213
         MVI   DDADSORG,X'40'     SEQUENTIAL                    GP14213
         MVI   DDARECFM,X'80'     RECFM=F                       GP14213
         MVC   DDALRECL,=AL2(44+96)    KEY+DATA                 GP14213
         MVC   DDABLKSI,=AL2(44+96)    KEY+DATA                 GP14213
         OI    DDWFLAG2,CWFVTOC   SET FOR VTOC ACCESS           GP14213
         MVC   DDWBLOCK,=A(DS1END-IECSDSL1+5)    SET BUFFER SZ  GP14213
         B     DDCSEQ             SKIP AROUND                   GP14213
         SPACE 1
*   For a DASD data set, obtain the format 1 DSCB to get DCB parameters
*   prior to OPEN. If the data set is not found, the DD may have
*   used an alias name for the data set. If so, we look it up in the
*   catalog, and use the true name to try again.
*
NOTVTOC  L     R14,CAMDUM         GET FLAGS IN FIRST WORD       GP14205
         LA    R15,JFCBDSNM       POINT TO DSN                  GP14205
         LA    R0,UCBVOLI         POINT TO SERIAL               GP14205
         LA    R1,DS1FMTID        POINT TO RETURN               GP14205
         STM   R14,R1,CAMLST                                    GP14205
         MVI   OPERF,ORFNODS1     PRESET FOR BAD DSCB 1         GP14205
         OBTAIN CAMLST       READ DSCB1                         GP14205
         BXLE  R15,R15,OBTGOOD                                  GP14233
         SPACE 1
         TM    JFCBTSDM,JFCCAT    Cataloged DS ?                GP14233
         BZ    TESTORGA             No; fail unless VSAM        GP14233
         MVC   TRUENAME,JFCBDSNM  Copy name in case replaced    GP14233
         L     R14,CAMLOC         GET FLAGS IN FIRST WORD       GP14233
         LA    R15,TRUENAME       POINT TO DSN                  GP14233
         LA    R0,0                 CVOL pointer                GP14233
         LA    R1,CATWORK         POINT TO RETURN               GP14233
         STM   R14,R1,CAMLST                                    GP14233
         OBTAIN CAMLST       CHECK CATALOG ENTRY                GP14233
         BXH   R15,R15,OPSERR                                   GP14233
         L     R14,CAMDUM         GET FLAGS IN FIRST WORD       GP14233
         LA    R15,TRUENAME       POINT TO DSN                  GP14233
         LA    R0,UCBVOLI         POINT TO SERIAL               GP14233
         LA    R1,DS1FMTID        POINT TO RETURN               GP14233
         STM   R14,R1,CAMLST                                    GP14233
         OBTAIN CAMLST       READ DSCB1                         GP14233
         BXLE  R15,R15,OBTGOOD                                  GP14233
TESTORGA TM    JFCDSRG2,JFCORGAM  VSAM?                         GP14233
         BNZ   DDCVSAM              Yes; let open handle it     GP14233
         B     OPSERR                                           GP14233
OBTGOOD  CLI   DS1FMTID,C'1'      SUCCESSFUL READ ?             GP14205
         BNE   OPSERR               NO; OOPS                    GP14205
         CLC   =C'IBM',FM1SMSFG   Old format ?                  GP14205
         BNE   *+4+6                No; keep intact             GP14205
         MVC   FM1SMSFG(2),ZEROES      Fake it out              GP14205
         SPACE 1                                                GP14205
         CLC   DDADSORG,ZEROES                                  GP14205
         BNE   *+4+6                                            GP14205
         MVC   DDADSORG,DS1DSORG  SAVE                          GP14205
         CLI   DDARECFM,0                                       GP14205
         BNE   *+4+6                                            GP14205
         MVC   DDARECFM,DS1RECFM    DCB                         GP14205
         CLC   DDALRECL,ZEROES                                  GP14205
         BNE   *+4+6                                            GP14205
         MVC   DDALRECL,DS1LRECL      ATTRIBUTES                GP14205
         CLC   DDABLKSI,ZEROES                                  GP14205
         BNE   *+4+6                                            GP14205
         MVC   DDABLKSI,DS1BLKL                                 GP14205
         SPACE 1                                                GP14205
         LTR   R5,R5              DID JFCB HAVE OVERRIDING BUFFER SIZE
         BNZ   DDCUJBLK             YES; IGNORE DSCB            GP14205
         LH    R5,DS1BLKL         GET BLOCK SIZE                GP14205
         C     R5,DDWBLOCK        COMPARE TO PRIOR VALUE        GP14205
         BNH   DDCUJBLK             NOT LARGER                  GP14205
         ST    R5,DDWBLOCK        SAVE FOR RETURN               GP14205
DDCUJBLK TM    DS1DSORG+1,DS1ACBM VSAM ?                        GP14233
         BNZ   DDCVSAM              YES; HOP FOR THE BEST       GP14233
         CLI   DS1DSORG+1,0       ANYTHING UNPROCESSABLE?       GP14205
         BNE   BADDSORG                                         GP14205
         TM    DS1DSORG,255-DS1DSGPS-DS1DSGPO-DS1DSGU NOT GOOD  GP14251
         BNZ   BADDSORG             YES; TOO BAD                GP14205
         TM    DS1DSORG,DS1DSGPO                                GP14205
         BZ    DDCSEQ             (CHECK JFCB OVERRIDE DSORG?)  GP14205
         TM    JFCBIND1,JFCPDS    MEMBER NAME ON DD ?           GP14205
         BNZ   DDCPMEM              YES; SHOW                   GP14205
         OI    DDWFLAG2,CWFPDS    SET PDS ONLY                  GP14205
         B     DDCKPDS              Test LSTAR & SMS            GP14205
DDCVSAM  OI    DDWFLAG2,CWFVSM    SHOW VSAM MODE                GP14233
         B     DDCX1DD              NEXT DD                     GP14233
DDCPMEM  OI    DDWFLAG2,CWFPDQ    SHOW SEQUENTIAL PDS USE       GP14205
DDCKPDS  DS    0H                                               GP14205
         AIF   ('&ZSYS' EQ 'S390').SMSOK                        GP14205
         TM    FM1SMSFG,FM1STRP+FM1PDSEX+FM1DSAE  Usable        GP14205
         BNZ   BADDSORG             No; too bad                 GP14205
         LA    R0,ORFBDPDS        Preset - not initialized      GP14205
         ICM   R15,7,DS1LSTAR     Any directory blocks?         GP14205
         BZ    OPRERR               No; fail                    GP14205
         AGO   .SMSCOM            Skip SMS tests                GP14205
.SMSOK   TM    FM1SMSFG,FM1STRP+FM1PDSEX  HFS or Ext. format?   GP14205
         BNZ   BADDSORG             Yes; can't handle           GP14205
.SMSCOM  B     DDCX1DD                                          GP14205
         SPACE 1
*---------------------------------------------------------------------*
*   FOR A CONCATENATION, PROCESS THE NEXT DD                          *
*---------------------------------------------------------------------*
DDCSEQ   OI    DDWFLAG2,CWFSEQ    SET FOR SEQUENTIAL ACCESS     GP14205
DDCX1DD  SR    R0,R0              RESET                         GP14205
         LTR   R8,R8              FIRST TIME FOR DD ?           GP14205
         BZ    FIND1DD              NO                          GP14205
         MVC   DDWFLAG1,DDWFLAG2  SAVE FLAGS FOR FIRST DD       GP14205
         MVI   DDWFLAG2,0         RESET DD                      GP14205
         SR    R8,R8              SIGNAL FIRST DD DONE          GP14205
FIND1DD  IC    R0,TIOELNGH        GET ENTRY LENGTH BACK         GP14205
         AR    R9,R0              NEXT ENTRY                    GP14205
         ICM   R0,1,TIOELNGH      GET NEW ENTRY LENGTH          GP14205
         BZ    DDCTDONE             ZERO; ALL DONE              GP14205
         TM    TIOESTTA,TIOSLTYP  SCRATCHED ENTRY?              GP14205
         BNZ   DDCTDONE             YES; DONE (?)               GP14205
         CLI   TIOEDDNM,C' '      CONCATENATION?                GP14205
         BH    DDCTDONE             NO; DONE WITH DD            GP14205
         LA    R4,DDASIZE(,R4)    NEXT DD ATTRIBUTE ENTRY       GP14205
         SR    R7,R7                                            GP14205
         ICM   R7,7,TIOEFSRT      LOAD UCB ADDRESS (COULD BE ZERO)
         USING UCBOB,R7                                         GP14205
         MVI   OPERF,ORFBATIO     SET FOR INVALID TIOT          GP14205
         CLI   TIOELNGH,20        SINGLE UCB ?                  GP14205
         BL    OPSERR               NOT EVEN                    GP14205
         B     DDCHECK            AND PROCESS IT                GP14205
         SPACE 1                                                GP14205
BADDSORG LA    R0,ORFBADSO        BAD DSORG                     GP14205
         B     OPRERR                                           GP14205
         SPACE 2                                                GP14205
***********************************************************************
         POP   USING                                            GP14205
         SPACE 1                                                GP14205
DDCTDONE MVC   DDWFLAGS,DDWFLAG1  COPY FIRST DD'S FLAGS         GP14205
         NI    DDWFLAGS,255-CWFDD BUT RESET FIRST DD PRESENT    GP14205
         OC    DDWFLAGS,DDWFLAG2  OR TOGETHER                   GP14205
         AMUSE ,                  RESTORE AMODE FROM ENTRY      GP14205
* Note that R5 is used as a scratch register
         L     R8,PARM4           R8 POINTS TO LRECL
* PARM5    has BLKSIZE
* PARM6    has ZBUFF2 pointer
         SPACE 1
         LDVAL R4,PARM2           MODE.  0=input 1=output, etc. GP14251
         SPACE 1
*   Conditional forms of storage acquisition are reentrant unless
*     they pass values that vary between uses, which ours don't,
*     or require storaage alteration (ditto).
         AIF   ('&ZSYS' NE 'S390').NOLOW
*OLD*    GETMAIN R,LV=ZDCBLEN,SP=SUBPOOL,LOC=BELOW  no return code
         STORAGE OBTAIN,LENGTH=ZDCBLEN,SP=SUBPOOL,LOC=BELOW,COND=YES
         AGO   .FINLOW
*   USE ,BNDRY=PAGE FOR EASIER DEBUGGING
.NOLOW   GETMAIN RC,LV=ZDCBLEN,SP=SUBPOOL BNDRY=PAGE            GP14244
*   Note that PAGE alignment makes for easier dump reading
*   but wastes storage - so we use it for debugging only.
*   Using RC to get a return code if memory unavailable.
*
.FINLOW  LA    R0,ORFNOSTO        Preset for no storage         GP14205
         BXH   R15,R15,OPRERR       Return error code           GP14205
         LR    R10,R1             Addr.of storage obtained to its base
         USING IHADCB,R10         Give assembler DCB area base register
         LR    R0,R10             Load output DCB area address
         LA    R1,ZDCBLEN         Load output length of DCB area
         LA    R15,0              Pad of X'00' and no input length
         MVCL  R0,R14             Clear DCB area to binary zeroes
         MVC   ZDDN,0(R3)         DDN for debugging             GP14205
         XC    ZMEM,ZMEM          Preset for no member          GP14205
         LDADD R14,PARM7          R14 POINTS TO MEMBER NAME (OF PDS)
         LA    R14,0(,R14)        Strip off high-order bit or byte
         LTR   R14,R14            Zero address passed ?         GP14205
         BZ    OPENMPRM             Yes; skip                   GP14205
         TM    0(R14),255-X'40'   Either blank or zero?         GP14205
         BZ    OPENMPRM             Yes; no member              GP14205
         MVC   ZMEM,0(R14)        Save member name              GP14205
*---------------------------------------------------------------------*
*   GET USER'S DEFAULTS HERE, BECAUSE THEY MAY GET CHANGED
*---------------------------------------------------------------------*
OPENMPRM L     R5,PARM3    HAS RECFM code (0-FB 1-VB 2-U)
         L     R14,0(,R5)         LOAD RECFM VALUE
         STC   R14,FILEMODE       PASS TO OPEN
         L     R14,0(,R8)         GET LRECL VALUE
         ST    R14,ZPLRECL        PASS TO OPEN                  GP14233
         L     R14,PARM5          R14 POINTS TO BLKSIZE
         L     R14,0(,R14)        GET BLOCK SIZE
         ST    R14,ZPBLKSZ        PASS TO OPEN                  GP14233
         MVC   ZDDFLAGS,DDWFLAGS  PRESERVE FLAGS (OPEN EXIT)    GP14244
         SPACE 1
*---------------------------------------------------------------------*
*   DO THE DEVICE TYPE NOW TO CHECK WHETHER EXCP IS POSSIBLE
*     ALSO BYPASS STUFF IF USER REQUESTED TERMINAL I/O
*---------------------------------------------------------------------*
OPCURSE  STC   R4,WWORK           Save to storage
         STC   R4,WWORK+1         Save to storage
         NI    WWORK+1,7          Retain only open mode bits
         TM    WWORK,IOFTERM      Terminal I/O ?
         BNZ   TERMOPEN           Yes; do completely different
***> Consider forcing terminal mode if DD is a terminal?
         MVC   DWDDNAM,0(R3)      Move below the line
         DEVTYPE DWDDNAM,DWORK    Check device type
         MVC   ZDVTYPE,DWORK+2    return device type            GP14251
         LA    R0,ORFNODD         Missing DD                    GP14251
         BXH   R15,R15,OPRERR     DD missing                    GP14251
         ICM   R0,15,DWORK+4      Any device size ?
         BNZ   OPHVMAXS
         MVC   DWORK+6(2),=H'32760'    Set default max
         SPACE 1
OPHVMAXS CLI   WWORK+1,3          Append (AKA Extend) requested ?
         BNE   OPNOTAP              No
         TM    DWORK+2,UCB3TAPE+UCB3DACC    TAPE or DISK ?
         BM    OPNOTAP              Yes; supported
         NI    WWORK,255-2        Change to plain output
*OR-FAIL BNM   FAILDCB              No, not supported
         SPACE 1
OPNOTAP  CLI   WWORK+1,2          UPDAT request?
         BNE   OPNOTUP              No
         LA    R0,ORFBDMOD        Preset for bad mode           GP14251
         CLI   DWORK+2,UCB3DACC   DASD ?
         BNE   OPRERR               No, not supported           GP14251
         SPACE 1
OPNOTUP  CLI   WWORK+1,4          INOUT or OUTIN ?
         BL    OPNOTIO              No
         TM    DWORK+2,UCB3TAPE+UCB3DACC    TAPE or DISK ?
         LA    R0,ORFBDMOD        Preset for bad mode           GP14251
         BNM   OPRERR               No; not supported           GP14251
         SPACE 1
OPNOTIO  TM    WWORK,IOFEXCP      EXCP requested ?
         BZ    OPFIXMD2             No
         CLI   DWORK+2,UCB3TAPE   TAPE/CARTRIDGE device?
         BE    OPFIXMD1             Yes; wonderful ?
OPFIXMD0 NI    WWORK,255-IOFEXCP  Cancel EXCP request
         B     OPFIXMD2
OPFIXMD1 L     R0,ZPBLKSZ         GET USER'S SIZE               GP14233
         CH    R0,=H'32760'       NEED EXCP ?
         BNH   OPFIXMD0             No; use BSAM
         ST    R0,DWORK+4         Increase max size
         ST    R0,ZPLRECL         ALSO RECORD LENGTH            GP14233
         MVI   FILEMODE,2         FORCE RECFM=U
         SPACE 1
OPFIXMD2 IC    R4,WWORK           Fix up
OPFIXMOD STC   R4,WWORK           Save to storage
         MVC   IOMFLAGS,WWORK     Save for duration
         SPACE 1
*---------------------------------------------------------------------*
*   Do as much common code for input and output before splitting
*   Set mode flag in Open/Close list
*   Move BSAM, QSAM, or EXCP DCB to work area
*---------------------------------------------------------------------*
         STC   R4,OPENCLOS        Initialize MODE=24 OPEN/CLOSE list
         NI    OPENCLOS,X'07'        For now
*                  OPEN mode: IN OU UP AP IO OI
         TR    OPENCLOS(1),=X'80,8F,84,8E,83,86,0,0'
         CLI   OPENCLOS,0         Unsupported ?
         LA    R0,ORFBDMOD        Preset bad mode               GP14251
         BE    OPRERR               Yes; fail                   GP14251
         SPACE 1
         TM    WWORK,IOFEXCP      (TAPE) EXCP mode ?
         BZ    OPQRYBSM             No
         MVC   ZDCBAREA(EXCPDCBL),EXCPDCB  Move DCB/IOB/CCW
         LA    R15,TAPEIOB   FOR EASIER SETTINGS
         USING IOBSTDRD,R15
         MVI   IOBFLAG1,IOBDATCH+IOBCMDCH   COMMAND CHAINING IN USE
         MVI   IOBFLAG2,IOBRRT2
         LA    R1,TAPEECB
         ST    R1,IOBECBPT
         LA    R1,TAPECCW
         ST    R1,IOBSTART   CCW ADDRESS
         ST    R1,IOBRESTR   CCW ADDRESS
         LA    R1,TAPEDCB
         ST    R1,IOBDCBPT   DCB
         LA    R1,TAPEIOB
         STCM  R1,7,DCBIOBAA LINK IOB TO DCB FOR DUMP FORM.ING
         LA    R0,1          SET BLOCK COUNT INCREMENT
         STH   R0,IOBINCAM
         DROP  R15
         B     OPREPCOM
         SPACE 1
OPQRYBSM TM    WWORK,IOFBLOCK     Block mode ?
*DEFUNCT BNZ   OPREPBSM
*DEFUNCT TM    WWORK,X'01'        In or Out
*DEFUNCT BNZ   OPREPQSM
OPREPBSM CLI   DDWFLAGS,CWFPDS    PDS ONLY (NO CONCAT?)         GP14244
         BE    OPREPBSQ             Yes; use for directory read GP14244
         TM    DDWFLAGS,CWFCONC+CWFSEQ   Seq. Concatenation?    GP14244
         BNZ   OPREPBSQ             Yes; plain BSAM             GP14244
         TM    DDWFLAGS,CWFCONC+CWFPDQ   Concatenation pds(MEM) GP14244
         BNZ   OPREPBSQ             YES; plain BSAM             GP14244
         CLI   DDWFLAGS,CWFPDQ    Single PDS with member?       GP14244
         BNE   OPREPBAM           Test PDS concatenation        GP14244
         CLI   WWORK+1,0          Plain input ?                 GP14244
         BNE   OPREPBSQ             No; leave it                GP14244
         B     OPREPBCM             Yes; will verify member     GP14244
OPREPBAM CLI   DDWFLAGS,X'90'     PDS CONCATENATION?            GP14205
         BNE   OPREPBSQ             NO                          GP14205
         OI    IOMFLAGS,IOFBPAM   Use BPAM logic                GP14205
         MVC   ZDCBAREA(BPAMDCBL),BPAMDCB  Move DCB template +  GP14205
         MVC   ZDCBAREA+BPAMDCBL(READDUML),READDUM    DECB      GP14205
         MVI   DCBRECFM,X'C0'     FORCE xSAM TO IGNORE          GP14205
         B     OPREPCOM                                         GP14205
OPREPBCM MVC   ZDCBAREA(BPAMDCBL),BPAMDCB  Move DCB template +  GP14205
         MVC   ZDCBAREA+BPAMDCBL(READDUML),READDUM    DECB      GP14205
         B     OPREPCOM                                         GP14205
OPREPBSQ MVC   ZDCBAREA(BSAMDCBL),BSAMDCB  Move DCB template to work
         MVC   ZDCBAREA+BPAMDCBL(READDUML),READDUM    DECB      GP14205
         TM    DWORK+2,UCB3DACC+UCB3TAPE    Tape or Disk ?
         BM    OPREPCOM           Either; keep RP,WP
         NC    DCBMACR(2),=AL1(DCBMRRD,DCBMRWRT) Strip Point
         B     OPREPCOM
         SPACE 1
OPREPQSM MVC   ZDCBAREA(QSAMDCBL),QSAMDCB   *> UNUSED <*
OPREPCOM MVC   DCBDDNAM,ZDDN              0(R3)
         MVC   DEVINFO(8),DWORK   Check device type
         ICM   R14,15,DEVINFO+4   Any ?                         GP14251
         LA    R0,ORFNODD         Set for no DD                 GP14251
         BZ    OPRERR             No DD card or ?               GP14251
         N     R4,=X'000000EF'    Reset block mode
         TM    DDWFLAGS,CWFVSM    VSAM ACCESS ?                 GP14233
         BNZ   OPDOVSAM                                         GP14233
         TM    DDWFLAGS,CWFVTOC   VTOC ACCESS ?                 GP14213
         BNZ   OPDOVTOC             USE STORAGE ONE             GP14213
         TM    WWORK,IOFTERM      Terminal I/O?
         BNZ   OPFIXMOD
         TM    WWORK,IOFBLOCK           Blocked I/O?
         BZ    OPREPJFC
         CLI   DEVINFO+2,UCB3UREC Unit record?
         BE    OPFIXMOD           Yes, may not block
         SPACE 1
OPREPJFC LA    R14,MYJFCB
* EXIT TYPE 07 + 80 (END OF LIST INDICATOR)
         ICM   R14,B'1000',=X'87'
         ST    R14,DCBXLST+4
         LA    R14,OCDCBEX        POINT TO DCB EXIT
* S370 EODAD assembled in DCB; fix for others                   GP14205
* Both S380 and S390 operate in 31-bit mode so need a stub
         AIF   ('&ZSYS' EQ 'S370').NODP24
         LA    R1,EOFR24
         STCM  R1,B'0111',DCBEODA
         MVC   EOFR24(EOFRLEN),ENDFILE   Put EOF code below the line
         ST    R14,DOPE31         Address of 31-bit exit
         OI    DOPE31,X'80'       Set high bit = AMODE 31
         MVC   DOPE24,DOPEX24     Move in stub code
         LA    R14,DOPE24         Switch to 24-bit stub
.NODP24  ICM   R14,8,=X'05'       SPECIFY DCB EXIT ADDRESS
         ST    R14,DCBXLST        AND SET IT BACK
         LA    R14,DCBXLST
         STCM  R14,B'0111',DCBEXLSA
         RDJFCB ((R10)),MF=(E,OPENCLOS)  Read JOB File Control Blk
*---------------------------------------------------------------------*
*   If the caller did not request EXCP mode, but the user has BLKSIZE
*   greater than 32760 on TAPE, then we set the EXCP bit in R4 and
*   restart the OPEN. Otherwise MVS should fail?
*   The system fails explicit BLKSIZE in excess of 32760, so we cheat.
*   The NCP field is not otherwise honored, so if the value is 32 to
*   64 inclusive, we use that times 1024 as a value (max 65535)
*---------------------------------------------------------------------*
         CLI   DEVINFO+2,UCB3TAPE TAPE DEVICE?
         BNE   OPNOTBIG           NO
         TM    WWORK,IOFEXCP      USER REQUESTED EXCP ?
         BNZ   OPVOLCNT           NOTHING TO DO
         CLI   JFCNCP,32          LESS THAN MIN ?
         BL    OPNOTBIG           YES; IGNORE
         CLI   JFCNCP,65          NOT TOO HIGH ?
         BH    OPNOTBIG           TOO BAD
*---------------------------------------------------------------------*
*   Clear DCB work area and force RECFM=U,BLKSIZE>32K
*     and restart the OPEN processing
*---------------------------------------------------------------------*
         LR    R0,R10             Load output DCB area address
         LA    R1,ZDCBLEN         Load output length
         LA    R15,0              Pad of X'00'
         MVCL  R0,R14             Clear DCB area to zeroes
         SR    R0,R0
         ICM   R0,1,JFCNCP        NUMBER OF CHANNEL PROGRAMS
         SLL   R0,10              *1024
         C     R0,=F'65535'       LARGER THAN CCW SUPPORTS?
         BL    *+8                NO
         L     R0,=F'65535'       LOAD MAX SUPPORTED
         ST    R0,ZPBLKSZ         MAKE NEW VALUES THE DEFAULT   GP14233
         ST    R0,ZPLRECL         MAKE NEW VALUES THE DEFAULT   GP14233
         MVI   FILEMODE,2         USE RECFM=U
         LA    R0,IOFEXCP         GET EXCP OPTION
         OR    R4,R0              ADD TO USER'S REQUEST
         B     OPCURSE            AND RESTART THE OPEN
         SPACE 1
OPVOLCNT SR    R1,R1
         ICM   R1,1,JFCBVLCT      GET VOLUME COUNT FROM DD
         BNZ   *+8                OK
         LA    R1,1               SET FOR ONE
         ST    R1,ZXCPVOLS        SAVE FOR EOV
         SPACE 1
OPNOTBIG CLI   DEVINFO+2,UCB3DACC   Is it a DASD device?
         BNE   OPNODSCB           No; no member name supported
*---------------------------------------------------------------------*
*   For a DASD resident file, get the format 1 DSCB
*---------------------------------------------------------------------*
*
         CLI   DS1FMTID,C'1'      Already done?                 GP14205
         BE    OPNODSCB             Yes; save time              GP14205
* CAMLST CAMLST SEARCH,DSNAME,VOLSER,DSCB+44
         L     R14,CAMDUM         Get CAMLST flags
         LA    R15,JFCBDSNM       Load address of output data set name
         LA    R0,JFCBVOLS        Load addr. of output data set volser
         LA    R1,DS1FMTID        Load address of where to put DSCB
         STM   R14,R1,CAMLST      Complete CAMLST addresses
         OBTAIN CAMLST            Read the VTOC record
         MVI   OPERF,ORFNODS1     PRESET FOR BAD DSCB 1
         LTR   R15,R15            Check return                  GP14205
         BNZ   OPSERR               Bad; fail                   GP14205
         B     OPNODSCB
         SPACE 1
OPDOVTOC OPENCALL OPENVTOC   Invoke VTOC open code              GP14233
         B     GETBUFF       Normal return                      GP14233
         SPACE 1
*---------------------------------------------------------------------*
*   VSAM:  OPEN ACB; SET RPL; SET FLAGS FOR SEQUENTIAL READ OR WRITE  *
*---------------------------------------------------------------------*
OPDOVSAM OPENCALL OPENVSAM        Transfer to the extension     GP14233
         B     GETBUFF            Join common code              GP14233
         SPACE 1
*---------------------------------------------------------------------*
*   Split READ and WRITE paths
*     Note that all references to DCBRECFM, DCBLRECL, and DCBBLKSI
*     have been replaced by ZARECFM, ZPLRECL, and ZPBLKSZ for EXCP use.
*---------------------------------------------------------------------*
OPNODSCB TM    WWORK,1            See if OPEN input or output
         BNZ   OPENWRIT
         MVI   OPERF,ORFBACON     Preset invalid concatenation  GP14205
         TM    DDWFLAGS,CWFDD     Concatenation ?               GP14205
         BZ    READNCON             No                          GP14205
         TM    OPENCLOS,X'07'     Other than simple open?
         BNZ   OPSERR               Yes, fail
*---------------------------------------------------------------------*
*
* READING
*   N.B. moved RDJFCB prior to member test to allow uniform OPEN and
*        other code. Makes debugging and maintenance easier
*
*---------------------------------------------------------------------*
READNCON OI    JFCBTSDM,JFCNWRIT  Don't mess with control block
         CLI   DEVINFO+2,UCB3DACC   Is it a DASD device?
         BNE   OPENVSEQ           No; no member name supported
*---------------------------------------------------------------------*
* See if DSORG=PO but no member; use member from JFCB if one
*---------------------------------------------------------------------*
         TM    DDWFLAG2,CWFDD     Concatenation?                GP14205
         BZ    OPENITST             No                          GP14205
         TM    DDWFLAGS,CWFSEQ+CWFPDQ  Sequential ?             GP14205
         BNZ   OPENVSEQ             Yes; bypass member check    GP14205
         B     OPENBCOM                                         GP14205
         SPACE 1
OPENITST TM    DDWFLAGS,CWFSEQ    Single DD non-PDS?            GP14205
         BNZ   OPENVSEQ             Yes; skip PDS stuff         GP14205
         TM    DDWFLAGS,CWFPDQ    PDS with member ?             GP14205
         BZ    OPENBCOM             No                          GP14251
         TM    WWORK,X'01'        Output?                       GP14251
         BNZ   OPENVSEQ             Yes; skip PDS stuff         GP14205
         B     OPENBINP             Yes; check input mode       GP14205
OPENBCOM TM    DS1DSORG,DS1DSGPO  See if DSORG=PO
         BZ    OPENVSEQ           Not PDS, don't read PDS directory
OPENBINP TM    WWORK,X'07'   ANY NON-READ OPTION ?
         LA    R0,ORFBDMOD          Yes; not supported          GP14251
         BNZ   OPRERR               No, fail                    GP14251
         TM    ZMEM,255-X'40'     See if a member name          GP14205
         BNZ   ZEROMEM            User member - reset JFCB      GP14205
         TM    JFCBIND1,JFCPDS    See if a member name in JCL
         BZ    OPENDIR            No; read directory
         MVC   ZMEM,JFCBELNM      Save the member name          GP14205
ZEROMEM  NI    JFCBIND1,255-JFCPDS    Reset it
         XC    JFCBELNM,JFCBELNM  Delete it in JFCB
         B     OPENMEM            Change DCB to BPAM PO
*---------------------------------------------------------------------*
* At this point, we have a PDS but no member name requested.
* Request must be to read the PDS directory
*---------------------------------------------------------------------*
OPENDIR  TM    OPENCLOS,X'0F'     Other than plain OPEN ?
         LA    R0,ORFBDMOD          Yes; not supported          GP14251
         BNZ   OPRERR               No, fail (allow UPDAT later?)
         LA    R0,256             Set size for Directory BLock
         STH   R0,DCBBLKSI        Set DCB BLKSIZE to 256
         STH   R0,DCBLRECL        Set DCB LRECL to 256
         ST    R0,ZPLRECL                                       GP14233
         ST    R0,ZPBLKSZ                                       GP14233
         MVI   DCBRECFM,DCBRECF   Set DCB RECFM to RECFM=F (notU?)
         B     OPENIN
OPENMEM  TM    DDWFLAGS,CWFDD     Concatenation ?               GP14205
         BZ    OPENBPAM             No; use BPAM                GP14205
         TM    DDWFLAGS,CWFSEQ CWFPDQ  Rest sequential ?        GP14205
         BNZ   OPENVSEQ             Must use BSAM               GP14205
OPENBPAM OI    JFCBTSDM,JFCVSL    Force OPEN analysis of JFCB
         MVI   DCBDSRG1,DCBDSGPO  Replace DCB DSORG=PS with PO
         B     OPENIN
         SPACE 1
OPENVSEQ TM    ZMEM,255-X'40'     Member name for sequential?   GP14205
         LA    R0,ORFBDMEM        Member not allowed            GP14251
         BNZ   OPRERR             Yes, fail                     GP14251
         TM    DDWFLAGS,CWFSEQ+CWFPDQ  SEQUENTIAL ACCESS ?   *TEST
         BZ    OPENIN               NO; SKIP CONCAT          *TEST
         MVI   DCBMACR+1,0        Remove Write                  GP14205
         OI    DCBOFLGS,DCBOFPPC  Allow unlike concatenation
OPENIN   OPEN  MF=(E,OPENCLOS),TYPE=J  Open the data set

         TM    DCBOFLGS,DCBOFOPN  Did OPEN work?
         BZ    FAILDCB            OPEN failed, go return error code -37
         TM    ZMEM,255-X'40'     Member name?                  GP14205
         BZ    GETBUFF            No member name, no find       GP14205
*  N.B. BLDL provides the concatenation number.                 GP14205
*                                                               GP14205
         MVC   BLDLLIST(4),=AL2(1,12+2+31*2)                    GP14205
         MVC   BLDLNAME,ZMEM      Copy member name              GP14205
         BLDL  (R10),BLDLLIST     Try to find it                GP14205
         BXH   R15,R15,OPNOMEM    See if member found           GP14205
         TM    DDWFLAGS,CWFSEQ+CWFPDQ  SEQUENTIAL ACCESS ?      GP14229
         BNZ   SETMTTR              YES; LEAVE DCB INTACT       GP14229
*  SET DCB PARAMETERS FOR MEMBER'S PDS                          GP14205
         SR    R15,R15                                          GP14205
         IC    R15,PDS2CNCT-PDS2+BLDLNAME                       GP14205
         MH    R15,=AL2(DDASIZE)  Position to entry             GP14205
         LA    R15,DDWATTR-DDASIZE(R15) Point to start of table GP14205
         USING DDATTRIB,R15       Declare table entry           GP14205
         MVC   DCBRECFM,DDARECFM  Update                        GP14205
         LH    R0,DDALRECL                                      GP14205
         STH   R0,DCBLRECL                                      GP14205
         ST    R0,ZPLRECL                                       GP14205
         LH    R0,DDABLKSI                                      GP14205
         STH   R0,DCBBLKSI                                      GP14205
         ST    R0,ZPBLKSZ                                       GP14205
         SLR   R0,R0                                            GP14205
         IC    R0,DCBRECFM        Load RECFM                    GP14205
         STC   R0,ZRECFM                                        GP14205
         SRL   R0,6               Keep format only              GP14205
         STC   R0,FILEMODE        Store                         GP14205
         TR    FILEMODE,=AL1(1,1,0,2)  D,V,F,U                  GP14205
         MVC   RECFMIX,FILEMODE                                 GP14205
         TR    RECFMIX,=AL1(0,4,8,8)   F,V,U,U                  GP14205
         MVI   DCBRECFM,X'C0'     FORCE xSAM TO IGNORE          GP14205
         DROP  R15                                              GP14205
         SPACE 1
SETMTTR  FIND  (R10),ZMEM,D       Point to the requested member GP14205
         BXLE  R15,R15,GETBUFF    See if member found
         SPACE 1
* If FIND return code not zero, process return and reason codes and
* return to caller with a negative return code.
OPNOMEM  LR    R2,R15             Save error code               GP14205
         CLOSE MF=(E,OPENCLOS)    Close, FREEPOOL not needed
         LA    R7,2068            Set for member not found      GP14205
         CH    R2,=H'8'           FIND/BLDL RC=4 ?              GP14205
         BE    FREEDCB              Yes                         GP14205
         LA    R7,2064            Set for error                 GP14205
         B     FREEDCB
         SPACE 1
FAILDCB  LDVAL R4,PARM2           Reload mode                   GP14251
         N     R4,=F'1'           Mask other option bits
         LA    R7,37(R4,R4)       Preset OPEN error code
FREEDCB  L     R3,PARM1           R3 POINTS TO DDNAME           GP14213
         LR    R14,R13            COPY WORK AREA ADDRESS        GP14244
         LA    R15,OPENLEN-1(,R14)    LAST BYTE                 GP14251
         STM   R14,R15,OSNLIST                                  GP14244
         OI    OSNLIST+4,X'80'    SET END OF LIST               GP14244
         LTR   R14,R10            HAVE A ZDCBAREA ?             GP14244
         BZ    OSNDONE                                          GP14244
         LA    R15,ZDCBLEN-1(R14)    LAST BYTE                  GP14244
         STM   R14,R15,OSNLIST+8                                GP14244
         OI    OSNLIST+12,X'80'   SET END OF LIST               GP14244
         NI    OSNLIST+4,X'7F'    RESET SHORT END               GP14244
OSNDONE  L     R15,=A(@@SNAP)                                   GP14244
         LA    R1,OSNAP                                         GP14244
         BALR  R14,R15            CALL SNAPPER                  GP14244
         MVC   OPBADDD,0(R3)      SHOW WHAT                     GP14213
         WTO   'MVSSUPA - OPEN FAILED FOR nnnnnnnn',ROUTCDE=11  GP14213
OPBADDD  EQU   *-6-8,8,C'C'       Insert bad DD                 GP14213
FREEOSTO LTR   R10,R10            Should never happen           GP14233
         BZ    FREEDSTO             but it did during testing   GP14233
         FREEMAIN R,LV=ZDCBLEN,A=(R10),SP=SUBPOOL  Free DCB area
FREEDSTO LNR   R7,R7              Set return and reason code    GP14205
         B     RETURNOP           Go return to caller with negative RC
OSNLIST  DC    A(0,0,0,0)         Snap list: R13 work; R10 ZDCB GP14244
OSNHEAD  DC    A(OSNHEAD1,OSNHEAD2+X'80000000')  LABELS         GP14244
OSNHEAD1 DC    AL1(OSNHEAD2-*-1),C'AOPEN Work Area:'            GP14244
OSNHEAD2 DC    AL1(OSNHEAD3-*-1),C'ZDCBAREA:'                   GP14244
OSNHEAD3 EQU   *                  END OF HEADERS                GP14244
OSNAP    SNAP  PDATA=(PSW,REGS),LIST=OSNLIST,STRHDR=OSNHEAD,MF=L
         SPACE 1
@@BUGF   DC    F'0'          DEBUGGING FLAG                     GP14251
         SPACE 1
*---------------------------------------------------------------------*
*   Process for OUTPUT mode
*---------------------------------------------------------------------*
OPENWRIT MVI   OPERF,ORFBACON     Preset for invalid concatenation
         TM    DDWFLAG2,CWFDD     Concatenation other than input?
         BNZ   OPSERR               Yes, fail
         TM    ZMEM,255-X'40'     Member requested?             GP14205
         BZ    WNOMEM               No                          GP14205
         MVI   OPERF,ORFBDMEM     Preset for invalid member     GP14205
         CLI   DEVINFO+2,UCB3DACC   DASD ?
         BNE   OPSERR             Member name invalid           GP14205
         TM    DS1DSORG,DS1DSGPO  See if DSORG=PO
         BZ    OPSERR             Is not PDS, fail request      GP14205
         MVI   OPERF,ORFBDMOD     Preset for invalid MODE       GP14205
         TM    WWORK,X'06'        ANY NON-RITE OPTION ?
         BNZ   OPSERR               not allowed for PDS         GP14205
         MVC   JFCBELNM,ZMEM                                    GP14205
         OI    JFCBIND1,JFCPDS
         OI    JFCBTSDM,JFCVSL    Just in case
         B     WNOMEM2            Go to move DCB info
         SPACE 1
WNOMEM   TM    JFCBIND1,JFCPDS    See if a member name in JCL
         BO    WNOMEM2            Is member name, go to continue OPEN
* See if DSORG=PO but no member so OPEN output would destroy directory
         TM    DS1DSORG,DS1DSGPO  See if DSORG=PO
         BZ    WNOMEM2            Is not PDS, go OPEN
         MVC   BADMEMDD,ZDDN      Identify bad DD               GP14205
         WTO   'MVSSUPA - Output PDS missing member name for DD nnnnnnn*
               n',ROUTCDE=11                                    GP14205
BADMEMDD EQU   *-6-8,8,C'C'       Insert bad DD                 GP14205
         WTO   'MVSSUPA - Refuses to write over PDS directory',        C
               ROUTCDE=11
         ABEND 123                Abend without a dump
         SPACE 1
WNOMEM2  OPEN  MF=(E,OPENCLOS),TYPE=J
         TM    DCBOFLGS,DCBOFOPN  Did OPEN work?
         BZ    FAILDCB            OPEN failed, go return error code -39
         SPACE 1
*---------------------------------------------------------------------*
*   Acquire one BLKSIZE buffer for our I/O; and one LRECL buffer
*   for use by caller for @@AWRITE, and us for @@AREAD.
*---------------------------------------------------------------------*
GETBUFF  L     R5,ZPBLKSZ         Load the input blocksize      GP14233
         LA    R6,4(,R5)          Add 4 in case RECFM=U buffer
         L     R0,DDWBLOCK        Load the input blocksize      GP14205
         AH    R0,=H'4'           allow for BDW                 GP14205
         CR    R6,R0              Buffer size OK?               GP14205
         BNL   *+4+2                Yes                         GP14205
         LR    R6,R0              Use larger                    GP14205
         GETMAIN R,LV=(R6),SP=SUBPOOL  Get input buffer storage
         ST    R1,ZBUFF1          Save for cleanup
         ST    R6,ZBUFF1+4           ditto
         ST    R1,BUFFADDR        Save the buffer address for READ
         XC    0(4,R1),0(R1)      Clear the RECFM=U Record Desc. Word
         LA    R14,0(R5,R1)       Get end address
         ST    R14,BUFFEND          for real
         SPACE 1
         L     R6,ZPLRECL         Get record length
         LA    R6,4(,R6)          Insurance
         GETMAIN R,LV=(R6),SP=SUBPOOL  Get VBS build record area
         ST    R1,ZBUFF2          Save for cleanup
         ST    R6,ZBUFF2+4           ditto
         LA    R14,4(,R1)
         ST    R14,VBSADDR        Save the VBS read/user write
         L     R5,PARM6           Get caller's BUFFER address
         ST    R14,0(,R5)         and return work address
         AR    R1,R6              Add size GETMAINed to find end
         ST    R1,VBSEND          Save address after VBS rec.build area
         B     DONEOPEN           Go return to caller with DCB info
         SPACE 1
         PUSH  USING
*---------------------------------------------------------------------*
*   Establish ZDCBAREA for either @@AWRITE or @@AREAD processing to
*   a terminal, or SYSTSIN/SYSTERM in batch.
*---------------------------------------------------------------------*
TERMOPEN MVC   IOMFLAGS,WWORK     Save for duration
         NI    IOMFLAGS,IOFTERM+IOFOUT      IGNORE ALL OTHERS
         MVC   ZDCBAREA(TERMDCBL),TERMDCB  Move DCB/IOB/CCW
         MVC   ZIODDNM,0(R3)      DDNAME FOR DEBUGGING, ETC.
         TM    ZMEM,255-C' '      See if a member name          GP14251
         LA    R0,ORFBDMEM
         BNZ   OPRERR             Yes; fail                     GP14251
         L     R14,PSATOLD-PSA    GET MY TCB
         USING TCB,R14
         ICM   R15,15,TCBJSCB  LOOK FOR THE JSCB
         LA    R0,ORFNOJFC        Bad/missing system area       GP14251
         BZ    OPRERR        HUH ?                              GP14251
         USING IEZJSCB,R15
         ICM   R15,15,JSCBPSCB  PSCB PRESENT ?
         BZ    OPRERR        HUH ?                              GP14251
         L     R1,TCBFSA     GET FIRST SAVE AREA
         N     R1,=X'00FFFFFF'    IN CASE AM31
         L     R1,24(,R1)         LOAD INVOCATION R1
         USING CPPL,R1       DECLARE IT
         MVC   ZIOECT,CPPLECT
         MVC   ZIOUPT,CPPLUPT
         SPACE 1
         ICM   R6,15,ZPBLKSZ      Load the input blocksize      GP14233
         BP    *+12               Use it
         LA    R6,1024            Arbitrary non-zero size
         ST    R6,ZPBLKSZ         Return it                     GP14233
         ST    R6,ZPLRECL         Return it
         LA    R6,4(,R6)          Add 4 in case RECFM=U buffer
         GETMAIN R,LV=(R6),SP=SUBPOOL  Get input buffer storage
         ST    R1,ZBUFF2          Save for cleanup
         ST    R6,ZBUFF2+4           ditto
         LA    R1,4(,R1)          Allow for RDW if not V
         ST    R1,BUFFADDR        Save the buffer address for READ
         L     R5,PARM6           R5 points to ZBUFF2
         ST    R1,0(,R5)          save the pointer
         XC    0(4,R1),0(R1)      Clear the RECFM=U Record Desc. Word
         MVC   ZRECFM,FILEMODE    Requested format 0-2
         NI    ZRECFM,3           Just in case
         TR    ZRECFM,=X'8040C0C0'    Change to F / V / U
         MVI   IOIX,IXTRM         SET FOR TERMINMAL I/O         GP14213
         POP   USING
         SPACE 1
*   Lots of code tests DCBRECFM twice, to distinguish among F, V, and
*     U formats. We set the index byte to 0,4,8 to allow a single test
*     with a three-way branch.
DONEOPEN LR    R7,R10             Return DCB/file handle address
         LA    R0,IXUND
         TM    ZRECFM,DCBRECU     Undefined ?
         BO    SETINDEX           Yes
         BM    GETINDFV           No
         TM    ZRECFM,DCBRECTO    RECFM=D
         BZ    SETINDEX           No; treat as U
         B     SETINDVD
GETINDFV SR    R0,R0              Set for F
         TM    ZRECFM,DCBRECF     Fixed ?
         BNZ   SETINDEX           Yes
SETINDVD LA    R0,4               Preset for V
SETINDEX STC   R0,RECFMIX         Save for the duration
         L     R5,PARM3           Point to RECFM
         SRL   R0,2               change to major record format
         ST    R0,0(,R5)          Pass either RECFM F or V to caller
         L     R1,ZPLRECL         Load RECFM F or V max. record length
         ST    R1,0(,R8)          Return record length back to caller
         L     R5,PARM5           POINT TO BLKSIZE
         L     R0,ZPBLKSZ         Load RECFM U maximum record length
         ST    R0,0(,R5)          Pass new BLKSIZE
         L     R5,PARM2           POINT TO MODE
         MVC   0(4,R5),ZDVTYPE    DevTyp,RecFm,IOS+IOM flags    GP14251
*
* Finished with R5 now
*
RETURNOP FUNEXIT RC=(R7)          Return to caller
         SPACE 1
*---------------------------------------------------------------------*
*   RETURN ERROR CODE IN 2000 RANGE - SET IN CONCATENATION CHECK CODE *
*---------------------------------------------------------------------*
OPRERR   LR    R7,R0              CODE PASSED IN R0             GP14205
         B     OPSERR2                                          GP14205
OPSERR   SR    R7,R7              CLEAR FOR IC                  GP14205
         IC    R7,OPERF           GET ERROR FLAG                GP14205
OPSERR2  LA    R7,2000(,R7)       GET INTO RANGE                GP14205
         B     FREEDCB              AND RETURN WITH ERROR       GP14205
*
* This is not executed directly, but copied into 24-bit storage
ENDFILE  LA    R6,1               Indicate @@AREAD reached end-of-file
         LNR   R6,R6              Make negative
         BR    R14                Return to instruction after the GET
EOFRLEN  EQU   *-ENDFILE
*
         LTORG ,
         SPACE 1
         DS    0D                                               GP14205
BPAMDCB  DCB   MACRF=(R,W),DSORG=PO,DDNAME=BPAMDCB, input and output   *
               EXLST=1-1,EODAD=READEOD     DCB exits added later
BPAMDCBL EQU   *-BPAMDCB
         SPACE 1
         DS    0D                                               GP14205
BSAMDCB  DCB   MACRF=(RP,WP),DSORG=PS,DDNAME=BSAMDCB, input and output *
               EXLST=1-1,EODAD=READEOD     DCB exits added later
BSAMDCBL EQU   *-BSAMDCB
READDUM  READ  NONE,              Read record Data Event Control Block *
               SF,                Read record Sequential Forward       *
               ,       (R10),     Read record DCB address              *
               ,       (R8),      Read record input buffer             *
               ,       (R9),      Read BLKSIZE or 256 for PDS.Directory*
               MF=L               List type MACRO
READDUML EQU   *-READDUM                                        GP14205
         SPACE 1
         DS    0D                                               GP14205
EXCPDCB  DCB   DDNAME=EXCPDCB,MACRF=E,DSORG=PS,REPOS=Y,BLKSIZE=0,      *
               DEVD=TA,EXLST=1-1,RECFM=U
         DC    8XL4'0'         CLEAR UNUSED SPACE
         ORG   EXCPDCB+84    LEAVE ROOM FOR DCBLRECL
         DC    F'0'          VOLUME COUNT
PATCCW   CCW   1,2-2,X'40',3-3
         ORG   ,
EXCPDCBL EQU   *-EXCPDCB     PATTERN TO MOVE
         SPACE 1
TERMDCB  PUTLINE MF=L        PATTERN FOR TERMINAL I/O
TERMDCBL EQU   *-TERMDCB     SIZE OF IOPL
         SPACE 1
F65536   DC    F'65536'           Maximum VBS record GETMAIN length
*
* QSAMDCB changes depending on whether we are in LOCATE mode or
* MOVE mode
QSAMDCB  DCB   MACRF=P&OUTM.M,DSORG=PS,DDNAME=QSAMDCB,EODAD=READEOD
QSAMDCBL EQU   *-QSAMDCB
*
*
* CAMDUM CAMLST SEARCH,DSNAME,VOLSER,DSCB+44
CAMDUM   CAMLST SEARCH,*-*,*-*,*-*
CAMLEN   EQU   *-CAMDUM           Length of CAMLST Template
CAMLOC   CAMLST NAME,*-*,*-*,*-*       look in catalog          GP14233
         SPACE 1
         ORG   CAMLOC+4           Don't need rest               GP14233
         POP   USING
         SPACE 1
*---------------------------------------------------------------------*
*   Expand OPEN options for reference
*---------------------------------------------------------------------*
ADHOC    DSECT ,
OPENREF  OPEN  (BSAMDCB,INPUT),MF=L    QSAM, BSAM, any DEVTYPE
         OPEN  (BSAMDCB,OUTPUT),MF=L   QSAM, BSAM, any DEVTYPE
         OPEN  (BSAMDCB,UPDAT),MF=L    QSAM, BSAM, DASD
         OPEN  (BSAMDCB,EXTEND),MF=L   QSAM, BSAM, DASD, TAPE
         OPEN  (BSAMDCB,INOUT),MF=L          BSAM, DASD, TAPE
         OPEN  (BSAMDCB,OUTINX),MF=L         BSAM, DASD, TAPE
         OPEN  (BSAMDCB,OUTIN),MF=L          BSAM, DASD, TAPE
         SPACE 1
PARMSECT DSECT ,             MAP CALL PARM
PARM1    DS    A             FIRST PARM     DD NAME
PARM2    DS    A              NEXT PARM     I/O MODE
PARM3    DS    A              NEXT PARM     FORMAT (F, V, U)
PARM4    DS    A              NEXT PARM     RECORD LEN
PARM5    DS    A              NEXT PARM     BLOCK SIZE
PARM6    DS    A              NEXT PARM     opt. BUFFER
PARM7    DS    A              NEXT PARM     MEMBER NAME
PARM8    DS    A              NEXT PARM
         SPACE 1
DDATTRIB DSECT ,
DDADSORG DS    H             DS ORG FROM JFCB OR DSCB1 (2B)
DDARECFM DS    X             RECORD FORMAT
DDAATTR  DS    X             ATTRIBUTES (UCBRPS)
DDALRECL DS    H             RECORD LENGTH
DDABLKSI DS    H             BLOCK/BUFFER SIZE
DDASIZE  EQU   *-DDATTRIB
MVSSUPA  CSECT ,
         SPACE 2
***********************************************************************
*                                                                     *
*    OPEN DCB EXIT - if RECFM, LRECL, BLKSIZE preset, no change       *
*                     unless forced by device (e.g., unit record      *
*                     not blocked)                                    *
*                    for PDS directory read, F, 256, 256 are preset.  *
*    a) device is unit record - default U, device size, device size   *
*    b) all others - default to values passed to AOPEN                *
*                                                                     *
*    For FB, if LRECL > BLKSIZE, make LRECL=BLKSIZE                   *
*    For VB, if LRECL+3 > BLKSIZE, set spanned                        *
*                                                                     *
*    So, what this means is that if the DCBLRECL etc fields are set   *
*    already by MVS (due to existing file, JCL statement etc),        *
*    then these aren't changed. However, if they're not present,      *
*    then start using the "LRECL" etc previously set up by C caller.  *
*                                                                     *
*    Note that the exit runs in the caller's AMODE, and does not      *
*    switch modes (hence return is plain BR R14)                      *
*                                                                     *
***********************************************************************
         PUSH  USING
         DROP  ,
         USING OCDCBEX,R15
         USING IHADCB,R10    DECLARE OUR DCB WORK SPACE         GP14205
OCDCBEX  LR    R12,R15       MAKE SAFE BASE                     GP14205
         DROP  R15                                              GP14205
         USING OCDCBEX,R12   NEW BASE                           GP14205
         LR    R11,R14       SAVE RETURN                        GP14205
         LR    R10,R1        SAVE DCB ADDRESS AND OPEN FLAGS    GP14205
         N     R10,=X'00FFFFFF'   NO 0C4 ON DCB ACCESS IF AM31  GP14205
         TM    IOPFLAGS,IOFDCBEX  Been here before ?
         BZ    OCDCBX1            No; nothing on first entry
         TM    ZDDFLAGS,CWFSEQ+CWFPDQ  SEQUENTIAL ACCESS?       GP14244
         BZ    OCDCBX1            No; nothing to do             GP14244
         OI    IOPFLAGS,IOFCONCT  Set unlike concatenation
         OI    DCBOFLGS,DCBOFPPC  Keep them coming
         TM    DCBRECFM,X'E0'     Any RECFM?                    GP14205
         BZ    OCDCBX1              No; use previous            GP14205
         SLR   R0,R0                                            GP14205
         IC    R0,DCBRECFM        Load RECFM                    GP14205
         SRL   R0,6               Keep format only              GP14205
         STC   R0,FILEMODE        Store                         GP14205
         TR    FILEMODE,=AL1(1,1,0,2)  D,V,F,U                  GP14205
         MVC   RECFMIX,FILEMODE                                 GP14205
         TR    RECFMIX,=AL1(0,4,8,8)   F,V,U,U                  GP14205
         MVC   ZPLRECL+2(2),DCBLRECL                            GP14205
         MVC   ZPBLKSZ+2(2),DCBBLKSI                            GP14205
OCDCBX1  OI    IOPFLAGS,IOFDCBEX  Show exit entered
         SR    R2,R2         FOR POSSIBLE DIVIDE (FB)
         SR    R3,R3
         ICM   R3,3,DCBBLKSI   GET CURRENT BLOCK SIZE
         SR    R4,R4         FOR POSSIBLE LRECL=X
         ICM   R4,3,DCBLRECL GET CURRENT RECORD LENGTH
         NI    FILEMODE,3    MASK FILE MODE
         MVC   ZRECFM,FILEMODE   GET OPTION BITS
         TR    ZRECFM,=X'90,50,C0,C0'  0-FB  1-VB  2-U
         TM    DCBRECFM,DCBRECLA  ANY RECORD FORMAT SPECIFIED?
         BNZ   OCDCBFH       YES
         CLI   DEVINFO+2,UCB3UREC  UNIT RECORD?
         BNE   OCDCBFM       NO; USE OVERRIDE
OCDCBFU  CLI   FILEMODE,0         DID USER REQUEST FB?
         BE    OCDCBFM            YES; USE IT
         OI    DCBRECFM,DCBRECU   SET U FOR READER/PUNCH/PRINTER
         B     OCDCBFH
OCDCBFM  MVC   DCBRECFM,ZRECFM
OCDCBFH  LTR   R4,R4
         BNZ   OCDCBLH       HAVE A RECORD LENGTH
         L     R4,DEVINFO+4       SET DEVICE SIZE FOR UNIT RECORD
         CLI   DEVINFO+2,UCB3UREC   UNIT RECORD?
         BE    OCDCBLH       YES; USE IT
*   REQUIRES CALLER TO SET LRECL=BLKSIZE FOR RECFM=U DEFAULT
         ICM   R4,15,ZPLRECL SET LRECL=PREFERRED BLOCK SIZE
         BNZ   *+8
         L     R4,DEVINFO+4  ELSE USE DEVICE MAX
         IC    R5,DCBRECFM   GET RECFM
         N     R5,=X'000000C0'  RETAIN ONLY D,F,U,V
         SRL   R5,6          CHANGE TO 0-D 1-V 2-F 3-U
         MH    R5,=H'3'      PREPARE INDEX
         SR    R6,R6
         IC    R6,FILEMODE   GET USER'S VALUE
         AR    R5,R6         DCB VS. DFLT ARRAY
*     DCB RECFM:       --D--- --V--- --F--- --U---
*     FILE MODE:       F V  U F V  U F  V U F  V U
         LA    R6,=AL1(4,0,-4,4,0,-4,0,-4,0,0,-4,0)  LRECL ADJUST
         AR    R6,R5         POINT TO ENTRY
         ICM   R5,8,0(R6)    LOAD IT
         SRA   R5,24         SHIFT WITH SIGN EXTENSION
         AR    R4,R5         NEW LRECL
         SPACE 1
*   NOW CHECK BLOCK SIZE
OCDCBLH  LTR   R3,R3         ANY ?
         BNZ   *+8           YES
         ICM   R3,15,ZPBLKSZ SET OUR PREFERRED SIZE             GP14233
         BNZ   *+8           OK
         L     R3,DEVINFO+4  SET NON-ZERO
         C     R3,DEVINFO+4  LEGAL ?
         BNH   *+8
         L     R3,DEVINFO+4  NO; SHORTEN
         TM    DCBRECFM,DCBRECU   U?
         BO    OCDCBBU       YES
         TM    DCBRECFM,DCBRECF   FIXED ?
         BZ    OCDCBBV       NO; CHECK VAR
         DR    R2,R4
         CH    R3,=H'1'      DID IT FIT ?
         BE    OCDCBBF       BARELY
         BH    OCDCBBB       ELSE LEAVE BLOCKED
         LA    R3,1          SET ONE RECORD MINIMUM
OCDCBBF  NI    DCBRECFM,255-DCBRECBR   BLOCKING NOT NEEDED
OCDCBBB  MR    R2,R4         BLOCK SIZE NOW MULTIPLE OF LRECL
         B     OCDCBXX       AND GET OUT
*   VARIABLE
OCDCBBV  LA    R5,4(,R4)     LRECL+4
         CR    R5,R3         WILL IT FIT ?
         BNH   *+8           YES
         OI    DCBRECFM,DCBRECSB  SET SPANNED
         B     OCDCBXX       AND EXIT
*   UNDEFINED
OCDCBBU  LR    R4,R3         FOR NEATNESS, SET LRECL = BLOCK SIZE
*   EXEUNT  (Save DCB options for EXCP compatibility in main code)
OCDCBXX  STH   R3,DCBBLKSI   UPDATE POSSIBLY CHANGED BLOCK SIZE
         STH   R4,DCBLRECL     AND RECORD LENGTH
         ST    R3,ZPBLKSZ    UPDATE POSSIBLY CHANGED BLOCK SIZE GP14233
         ST    R4,ZPLRECL      AND RECORD LENGTH                GP14233
         MVC   ZRECFM,DCBRECFM    DITTO
         LR    R14,R11  ACCOMODATE PAUL'S SUPERSTITION RE R14   GP15004
         BR    R14           RETURN TO OPEN                     GP15004
         POP   USING
         SPACE 2
         AIF   ('&ZSYS' EQ 'S370').NODOP24
***********************************************************************
*                                                                     *
*    OPEN DCB EXIT - 24 bit stub                                      *
*    This code is not directly executed. It is copied below the line  *
*    It is only needed for AMODE 31 programs (both S380 and S390      *
*    execute in this mode).                                           *
*                                                                     *
***********************************************************************
         PUSH  USING
         DROP  ,
         USING DOPEX24,R15
*
* This next line works because when we are actually executing,
* we are executing inside that DSECT, so the address we want
* follows the code. Also, it has already had the high bit set,
* so it will switch to 31-bit mode.
*
DOPEX24  L     R15,DOPE31-DOPE24(,R15)  Load 31-bit routine address
*
* The following works because while the AMODE is saved in R14, the
* rest of R14 isn't disturbed, so it is all set for a BSM to R15
*
         BSM   R14,R15                  Switch to 31-bit mode
DOPELEN  EQU   *-DOPEX24
         POP   USING
.NODOP24 ANOP  ,
         SPACE 2
***********************************************************************
*                                                                     *
*    AOPEN SUBROUTINES.                                               *
*    Code moved here to gain addressability.                          *
*                                                                     *
***********************************************************************
*   For VTOC access, we need to use OBTAIN and CAMLST for MVS 3.x     *
*     access, as CVAF services aren't available.                GP14213
***********************************************************************
OPENVTOC OSUBHEAD ,          Define extended entry              GP14233
         LA    R0,ORFBDMOD        Preset for bad mode           GP14213
         CLI   WWORK,0            Plain input ?                 GP14213
         OBRAN OPRERR,OP=BNE        No; fail                    GP14233
         LA    R0,ORFBACON        Preset invalid concatenation  GP14213
         TM    DDWFLAG2,CWFDD     Concatenation ?               GP14213
         OBRAN OPSERR,OP=BNZ        Yes, fail                   GP14233
         USING UCBOB,R7                                         GP14213
         L     R14,=A(CAMDUM)     GET FLAGS IN FIRST WORD       GP14233
         LA    R15,JFCBDSNM       POINT TO DSN                  GP14213
         LA    R0,UCBVOLI         POINT TO SERIAL               GP14213
         LA    R1,DS1FMTID        POINT TO RETURN               GP14213
         STM   R14,R1,CAMLST                                    GP14213
         MVI   OPERF,ORFNODS1     PRESET FOR BAD DSCB 1         GP14213
         OBTAIN CAMLST       READ DSCB1                         GP14213
         N     R15,=X'FFFFFFF8'   Other than 0 or 8?            GP14213
         OBRAN OPSERR,OP=BNZ        Too bad                     GP14233
         MVC   ZVLOCCHH(L'ZVLOCCHH+L'ZVHICCHH),DS4VTOCE+2       GP14213
         MVC   ZVUSCCHH,ZVLOCCHH  Set for scan start            GP14213
         MVI   ZVUSCCHH+L'ZVUSCCHH-1,0 Start with fmt 4 again   GP14213
         MVC   ZVHICCHH+L'ZVHICCHH-1,DS4DEVDT  Set end record   GP14213
         MVC   ZVCPVOL(L'ZVCPVOL+L'ZVTPCYL),DS4DEVSZ  Sizes     GP14213
         MVI   ZRECFM,X'80'       Set RECFM=F                   GP14213
         MVI   DCBRECFM,X'80'     Set RECFM=F                   GP14213
         LA    R0,DS1END-DS1DSNAM  DSCB size (44 key + 96 data) GP14213
         ST    R0,ZPBLKSZ         Treat key as part of data     GP14213
         ST    R0,ZPLRECL                                       GP14213
         STH   R0,DCBBLKSI                                      GP14213
         STH   R0,DCBLRECL                                      GP14213
         MVI   IOIX,IXVTC         Set for VTOC I/O              GP14213
         MVC   ZVSER,UCBVOLI      Remember the serial           GP14213
         OSUBRET ROUTE=      Return from extended entry         GP14233
         SPACE 2
***********************************************************************
*   VSAM OPEN support                                                 *
***********************************************************************
OPENVSAM OSUBHEAD ,          Define extended entry              GP14233
         LA    R0,ORFBDMOD        Preset for bad mode           GP14233
         TM    WWORK,X'06'        Plain input or output?        GP14363
         OBRAN OPRERR,OP=BNZ        No; fail for now            GP14233
         MVI   ZRECFM,X'C0'       Set RECFM=U                   GP14233
         MVI   RECFMIX,X'C0'        and access code             GP14233
         OI    IOSFLAGS,IOFVSAM   Use VSAM logic                GP14251
         MVI   IOIX,IXVSM         Set for VSAM I/O              GP14233
         LA    R0,ORFBACON        Preset invalid concatenation  GP14233
         TM    DDWFLAG2,CWFDD     Concatenation ?               GP14233
         OBRAN OPSERR,OP=BNZ        Yes, fail                   GP14233
         MVC   ZAACB(VSAMDCBL),VSAMDCB BUILD ACB                GP14233
         MVC   ACBDDNM-IFGACB+ZAACB(L'ACBDDNM),ZDDN             GP14233
         MVC   ZARPL(VSAMRPLL),VSAMRPL BUILD ACB                GP14233
         OPEN  ((R10)),MF=(E,OPENCLOS)                          GP14233
         SR    R7,R7                                            GP14233
         IC    R7,ACBERFLG-IFGACB+ZAACB  LOAD ERROR FLAG        GP14233
         LA    R7,1000(,R7)       Get into the 3000 range       GP14233
         LTR   R15,R15            Did it open?                  GP14233
         OBRAN OPSERR2,OP=BNZ     NO; TOO BAD                   GP14233
         SHOWCB ACB=(R10),AREA=(S,ZPKEYL),LENGTH=12,                   *
               FIELDS=(KEYLEN,RKP,LRECL),                              *
               MF=(G,ZASHOCB,ZASHOCBL)                          GP14233
         LTR   R15,R15           Did it work?                   GP14233
         OBRAN FAILDCB,OP=BNZ      No                           GP14233
         L     R0,ZPLRECL                                       GP14233
         ST    R0,ZPBLKSZ         Treat key as part of data     GP14233
         AH    R0,=H'4'           allow for a little extra      GP14233
         ST    R0,DDWBLOCK        Set the input blocksize       GP14233
         XC    ZARRN,ZARRN   CLEAR, JUST IN CASE                GP14233
         LA    R0,ZARRN                                         GP14233
         ST    R0,ZAARG      SET RRN/KEY TO NULL                GP14233
         LA    R2,ZARPL      Save RPL address                   GP14233
         MODCB RPL=(R2),ACB=(R10),OPTCD=(KEY,LOC,SEQ),                 *
               MF=(G,ZAMODCB,ZAMODCBL)                          GP14233
         LA    R3,ZAARG                                         GP14233
         ICM   R15,15,ZPKEYL TEST KEY LENGTH                    GP14233
         BZ    OPDOVMOD      PROCEED                            GP14233
         MODCB RPL=(R2),ARG=(R3),        OPTCD=(KEY,SEQ),              *
               MF=(G,ZAMODCB)                                   GP14233
OPDOVMOD TM    WWORK,1            Output?                       GP14233
         OBRAN GETBUFF,OP=BZ        No; get buffer              GP14233
         MODCB RPL=(R2),OPTCD=(ADR,SEQ,NUP,LOC),ARG=(R3),              *
               MF=(G,ZAMODCB)     Set for write                 GP14233
         OSUBRET ROUTE=      Return from extended entry         GP14233
VECTOR   OSUBRET ROUTE=(14)  Return from extended entry         GP14233
         SPACE 1
VSAMDCB  ACB   DDNAME=VSAMDCB,EXLST=EXLSTACB,                          *
               MACRF=(SEQ)                                      GP14233
VSAMDCBL EQU   *-VSAMDCB                                        GP14233
VSAMRPL  RPL   ACB=VSAMDCB,OPTCD=(SEQ,LOC,SYN)  read or write   GP14233
VSAMRPLL EQU   *-VSAMRPL                                        GP14233
EXLSTACB EXLST AM=VSAM,EODAD=VSAMEOD,LERAD=VLERAD,SYNAD=VSYNAD  GP14244
         SPACE 1                                                GP14233
         POP   USING                                            GP14233
         SPACE 2
***********************************************************************
*                                                                     *
*   This routine provided to enable cross-assembly of OS/390 & zOS    *
*   code under MVS 3.8. For a full featured system, use SWAREQ.       *
*                                                                     *
*                                                                     *
*   SWA LOOK-UP SUBROUTINE  (in older systems, just skips Q header    *
*    >> CALLER IN AMODE 31 FOR ATL access <<                          *
*        R1  - REQUESTED SVA ADDRESS/TOKEN; 24-BIT, RIGHT-JUSTIFIED   *
*        R15 - RETURNED SWA ADDRESS OR 0                              *
*        R14 - RETURN                                                 *
*                                                                     *
***********************************************************************
         PUSH  USING                                            GP15006
         USING LOOKSWA,R15                                      GP15006
LOOKSWA  DS    0H                                               GP15006
         AIF   ('&ZSYS' NE 'S390').LOOKSWA                      GP15006
         BSM   R14,0         Save callers AMODE                 GP15006
.LOOKSWA STM   R0,R3,12(R13)      Save regs                     GP15006
         N     R1,=X'00FFFFFF'    CLEAN IT                      GP15006
         AIF   ('&ZSYS' EQ 'S370' OR '&ZSYS' EQ 'S380').NOQMAT  GP15006
         SAM31 WORK=R2       Ensure 31-bit addressimg           GP15006
         L     R2,PSATOLD-PSA     Get TCB                       GP15006
         USING TCB,R2                                           GP15006
         L     R2,TCBJSCB                                       GP15006
*COMP*   USING IEZJSCB,R2                                       GP15006
*COMP*   L     R2,JSCBQMPI   (not in S370/380)                  GP15006
         L     R2,X'0F4'(,R2)     Get QMPI                      GP15006
*COMP*   USING IOPARAMS,R2                                      GP15006
         EX    R1,EXSWAODD   SEE WHETHER IT'S AN ODD ADDRESS    GP15006
         BZ    LOOKSVA       NO; HAVE ADDRESS                   GP15006
*COMP*   ICM   R2,15,QMAT    QMAT BASE                          GP15006
         ICM   R2,15,X'018'(R2)   Get QMAT                      GP15006
         BZ    LOOKSWA0      NO QMAT, SKIP IT                   GP15006
         SPACE 1                                                GP15006
*COMP*   USING QMAT,R2                                          GP15006
         LR    R0,R1         COPY TOKEN                         GP15006
         SRL   R0,16         MOVE EXTENT TO LAST BYTE           GP15006
         N     R1,=XL4'FFFF'   ISOLATE SVA OFFSET               GP15006
         LA    R3,X'FF'      MAX QMAT EXTENTS                   GP15006
         NR    R0,R3         ISOLATE QMAT COUNTER               GP15006
         BZ    LOOKSWAV      ZERO; CHECK QMAT VERSION           GP15006
         SPACE 1                                                GP15006
*COMP*AP ICM   R2,15,QMATNEXT  NEXT QMAT EXTENT                 GP15006
LOOKSWAP ICM   R2,15,X'00C'(R2) NXT QMAT EXTENT                 GP15006
         BZ    LOOKSWAX      NONE?                              GP15006
         BCT   R0,LOOKSWAP   LOOP TO FIND THE EXTENT            GP15006
         SPACE 1                                                GP15006
*COMP*AV CLI   QMATVERS,2    IS IT AN ESA4 QMAT?                GP15006
LOOKSWAV CLI   X'004'(R2),2  IS IT AN ESA4 QMAT?                GP15006
         BL    LOOKSWAX      NO, USE AS IS                      GP15006
         LA    R2,1(,R2)     ALIGN                              GP15006
LOOKSWAX ALR   R1,R2         ADD QMAT BASE                      GP15006
         L     R1,0(,R1)     GET HEADER ADDRESS                 GP15006
.NOQMAT  ANOP  ,                                                GP15006
LOOKSVA  LA    R15,16(,R1)   SKIP HEADER                        GP15006
LOOKSWAT LM    R0,R3,12(R13)     RESTORE CALLER'S REGISTERS     GP15006
         QBSM  0,R14         RETURN IN CALLER'S AMODE           GP15006
LOOKSWA0 SR    R15,R15       NOTHING FOUND - RETURN 0           GP15006
         B     LOOKSWAT      RETURN                             GP15006
EXSWAODD TM    =X'01',*-*    ODD ADDRESS?                       GP15006
         POP   USING                                            GP15006
         SPACE 2                                                GP14233
*
***********************************************************************
*                                                                     *
*  ALINE - See whether any more input is available                    *
*     R15=0 EOF     R15=1 More data available                         *
*                                                                     *
***********************************************************************
@@ALINE  FUNHEAD IO=YES,AM=YES,SAVE=(WORKAREA,WORKLEN,SUBPOOL)
         FIXWRITE ,
         TM    IOMFLAGS,IOFTERM   Terminal Input?
         BNZ   ALINEYES             Always one more?
         LR    R2,R10        PASS DCB                           GP14233
         LA    R3,KEPTREC
         LA    R4,KEPTREC+4
         STM   R2,R4,DWORK   BUILD PARM LIST
         LA    R15,@@AREAD
         LA    R1,DWORK
         BALR  R14,R15       GET NEXT RECORD
         SR    R15,R15       SET EOF FLAG
         LTR   R6,R6         HIT EOF ?
         BM    ALINEX        YES; RETURN ZERO
         OI    IOPFLAGS,IOFKEPT   SHOW WE'RE KEEPING A RECORD
ALINEYES LA    R15,1         ELSE RETURN ONE
ALINEX   FUNEXIT RC=(R15)
         SPACE 2
***********************************************************************
*                                                                     *
*  AREAD - Read from an open data set                                 *
*                                                                     *
*    R15 = 0  Record or block read; address and size returned         *
*    R15 = -1 Encountered End-of-File - no data returned              *
*    R15 = 4  Encountered new DD in unlike concatenation. No data     *
*               returned. Next read will be from new DD.              *
*                                                                     *
***********************************************************************
@@AREAD  FUNHEAD IO=YES,AM=YES,SAVE=SAVEADCB,US=NO   READ / GET
         L     R3,4(,R1)  R3 points to where to store record pointer
         L     R4,8(,R1)  R4 points to where to store record length
         SR    R0,R0
         ST    R0,0(,R3)          Return null in case of EOF
         ST    R0,0(,R4)          Return null in case of EOF
         FIXWRITE ,               For OUTIN request
         TM    IOPFLAGS,IOFLEOF   Prior EOF ?                   GP14213
         BNZ   READEOD            Yes; don't abend              GP14213
         SR    R6,R6              No EOF
         TM    IOPFLAGS,IOFKEPT   Saved record ?
         BZ    READREAD           No; go to read                GP14213
         LM    R8,R9,KEPTREC      Get prior address & length
         ST    R8,0(,R3)          Set address
         ST    R9,0(,R4)            and length
         XC    KEPTREC(8),KEPTREC Reset record info
         NI    IOPFLAGS,255-IOFKEPT   Reset flag                GP14213
         B     READGOOD                                         GP14244
         SPACE 1
READREAD SLR   R15,R15                                          GP14363
         IC    R15,IOIX           Branch by read type           GP14213
         B     *+4(R15)                                         GP14213
           B   REREAD               xSAM                        GP14213
           B   VSAMREAD             VSAM                        GP14213
           B   VTOCREAD             VTOC read                   GP14213
           B   TGETREAD             Terminal                    GP14213
         SPACE 1
*   Return here for end-of-block or unlike concatenation
*
REREAD   ICM   R8,B'1111',BUFFCURR  Load address of next record
         BNZ   DEBLOCK            Block in memory, go de-block it
         L     R8,BUFFADDR        Load address of input buffer
         L     R9,ZPBLKSZ         Load block size to read       GP14233
         CLI   RECFMIX,IXVAR      RECFM=Vxx ?
         BE    READ               No, deblock
         LA    R8,4(,R8)          Room for fake RDW
READ     SAM24 ,                  For old code
         TM    IOMFLAGS,IOFEXCP   EXCP mode?
         BZ    READBSAM           No, use BSAM
*---------------------------------------------------------------------*
*   EXCP read
*---------------------------------------------------------------------*
READEXCP STCM  R8,7,TAPECCW+1     Read buffer
         STH   R9,TAPECCW+6         max length
         MVI   TAPECCW,2          READ
         MVI   TAPECCW+4,X'20'    SILI bit
         EXCP  TAPEIOB            Read
         WAIT  ECB=TAPEECB        wait for completion
         TM    TAPEECB,X'7F'      Good ?
         BO    EXRDOK             Yes; calculate input length
         CLI   TAPEECB,X'41'      Tape Mark read ?
         BNE   EXRDBAD            NO
         CLM   R9,3,IOBCSW+5-IOBSTDRD+TAPEIOB  All unread?
         BNE   EXRDBAD            NO
         L     R1,DCBBLKCT
         BCTR  R1,0
         ST    R1,DCBBLKCT        allow for tape mark
         OI    DCBOFLGS,X'04'     Set tape mark found
         L     R0,ZXCPVOLS        Get current volume count
         SH    R0,=H'1'           Just processed one
         ST    R0,ZXCPVOLS
         BNP   READEOD            None left - take End File
         EOV   TAPEDCB            switch volumes
         B     READEXCP           and restart
         SPACE 1
EXRDBAD  ABEND 001,DUMP           bad way to show error?
         SPACE 1
EXRDOK   SR    R0,R0
         ICM   R0,3,IOBCSW+5-IOBSTDRD+TAPEIOB
         SR    R9,R0         LENGTH READ
         BNP   BADBLOCK      NONE ?
         AMUSE ,                  Restore caller's mode
         LTR   R6,R6              See if end of input data set
         BM    READEOD            Is end, go return to caller
         B     POSTREAD           Go to common code
         SPACE 1
*---------------------------------------------------------------------*
*   BSAM read   (also used for BPAM member read)                      *
*---------------------------------------------------------------------*
READBSAM SR    R6,R6              Reset EOF flag
         SAM24 ,                  Get low
         READ  DECB,              Read record Data Event Control Block C
               SF,                Read record Sequential Forward       C
               (R10),             Read record DCB address              C
               (R8),              Read record input buffer             C
               (R9),              Read BLKSIZE or 256 for PDS.DirectoryC
               MF=E               Execute a MF=L MACRO
*---------------------------------------------------------------------*
*                                                                     *
*   There is a stupid (?) error in the code. When processing unlike   *
*   PDS concatenations, the member is read correctly, but the EOF     *
*   triggers an S001 abend. To avoid this, we test prior to the CHECK *
*   and treat as an end-file.                                         *
*                                                                     *
*---------------------------------------------------------------------*
         TM    ZDDFLAGS,CWFSEQ+CWFPDQ  Sequential access ?      GP14244
         BNZ   READCHEK                  Yes; handle normally   GP14244
         WAIT  ECB=DECB                                         GP14244
         CLI   DECB,X'41'         Unit exception?               GP14244
         BNE   READCHEK             No; handle normally         GP14244
         L     R14,DECB+16    DECIOBPT
         USING IOBSTDRD,R14       Give assembler IOB base       GP14251
         CLI   IOBCSW+3,X'0D'     Unit exception?               GP14251
         BNE   READWACK           No; something else            GP14251
*TEST*   BE    READCHEK           Yes; let BPAM recover         GP14251
         DROP  R14                Don't need IOB address base anymore
         B     READEOD            Treat as EOF
*                                 If EOF, R6 will be set to F'-1'
READCHEK CHECK DECB               Wait for READ to complete
READWACK DS    0H                 *TEST*
         TM    IOPFLAGS,IOFCONCT  Did we hit concatenation?
         BZ    READUSAM           No; restore user's AM
         NI    IOPFLAGS,255-IOFCONCT   Reset for next time
         L     R5,ZPBLKSZ         Get block size                GP14205
         LA    R5,4(,R5)          Alloc for any BDW             GP14205
         C     R5,ZBUFF1+4        buffer sufficient?            GP14205
         BNH   READUNLK             yes; keep it                GP14205
         SPACE 1
*---------------------------------------------------------------------*
*   If the new concatenation requires a larger buffer, free the old
*   one and replace it by a larger one.                         GP14205
*---------------------------------------------------------------------*
         LM    R1,R2,ZBUFF1       Get buffer address and length GP14205
         FREEMAIN R,LV=(R2),A=(R1),SP=SUBPOOL  and free it      GP14205
         L     R5,ZPBLKSZ         Load the input blocksize      GP14205
         LA    R6,4(,R5)          Add 4 in case RECFM=U buffer  GP14205
         GETMAIN R,LV=(R6),SP=SUBPOOL  Get input buffer storage GP14205
         ST    R1,ZBUFF1          Save for cleanup              GP14205
         ST    R6,ZBUFF1+4           ditto                      GP14205
         ST    R1,BUFFADDR        Save the buffer address for READ
         XC    0(4,R1),0(R1)      Clear the RECFM=U Record Desc. Word
         LA    R14,0(R5,R1)       Get end address               GP14205
         ST    R14,BUFFEND          for real                    GP14205
READUNLK LA    R6,4          SET RETURN CODE FOR NEXT DS READ   GP14205
         B     READEXIT           Return code 4; read next call GP14205
         SPACE 1
READUSAM AMUSE ,                  Restore caller's mode
         LTR   R6,R6              See if end of input data set
         BM    READEOD            Is end, go return to caller
         L     R14,DECB+16    DECIOBPT
         USING IOBSTDRD,R14       Give assembler IOB base
         SLR   R1,R1              Clear residual amount work register
         ICM   R1,B'0011',IOBCSW+5  Load residual count
         DROP  R14                Don't need IOB address base anymore
         SR    R9,R1              Provisionally return blocklen
         STM   R8,R9,RSNLIST      Save for SNAP                 GP14244
         SPACE 1
POSTREAD TM    IOMFLAGS,IOFBLOCK  Block mode ?
         BNZ   POSTBLOK           Yes; process as such
         TM    ZRECFM,DCBRECU     Also exit for U
         BNO   POSTREED
POSTBLOK ST    R8,0(,R3)          Return address to user
         ST    R9,0(,R4)          Return length to user
         STM   R8,R9,KEPTREC      Remember record info
         XC    BUFFCURR,BUFFCURR  Show READ required next call
         B     READEXIT
POSTREED LA    R6,1               Error code - bad BDW          GP14244
         CLI   RECFMIX,IXVAR      See if RECFM=V
         BNE   EXRDNOTV           Is RECFM=U or F, so not RECFM=V
         ICM   R9,3,0(R8)         Get presumed block length
         C     R9,ZPBLKSZ         Valid?
         BH    BADBLOCK           No
         ICM   R0,3,2(R8)         Garbage in BDW?
         BNZ   BADBLOCK           Yes; fail
         LA    R6,2               Error code - bad RDW          GP14244
         B     EXRDCOM
EXRDNOTV LA    R0,4(,R9)          Fake length
         SH    R8,=H'4'           Space to fake RDW
         STH   R0,0(0,R8)         Fake RDW
         LA    R9,4(,R9)          Up for fake RDW (F/U)
EXRDCOM  LA    R8,4(,R8)          Bump buffer address past BDW
         SH    R9,=H'4'             and adjust length to match
         BM    BADBLOCK           Oops                          GP14244
         ST    R8,BUFFCURR        Indicate data available
         ST    R8,0(,R3)          Return address to user
         ST    R9,0(,R4)          Return length to user
         STM   R8,R9,KEPTREC      Remember record info
         LA    R7,0(R9,R8)        End address + 1
         ST    R7,BUFFEND         Save end
         SPACE 1
         TM    IOMFLAGS,IOFBLOCK   Block mode?
         BNZ   READGOOD           Yes; exit                     GP14244
         TM    ZRECFM,DCBRECU     Also exit for U
         BO    READGOOD                                         GP14244
*NEXT*   B     DEBLOCK            Else deblock
         SPACE 1
*        R8 has address of current record
DEBLOCK  CLI   RECFMIX,IXVAR      Is data set RECFM=U
         BL    DEBLOCKF           Is RECFM=Fx, go deblock it
*
* Must be RECFM=V, VB, VBS, VS, VA, VM, VBA, VBM, VSA, VSM, VBSA, VBSM
*  VBS SDW ( Segment Descriptor Word ):
*  REC+0 length 2 is segment length
*  REC+2 0 is record not segmented
*  REC+2 1 is first segment of record
*  REC+2 2 is last seqment of record
*  REC+2 3 is one of the middle segments of a record
*        R5 has address of current record
DEBLOCKV CLI   0(R8),X'80'   LOGICAL END OF BLOCK ?
         BE    REREAD        YES; DONE WITH THIS BLOCK
         LH    R9,0(,R8)     GET LENGTH FROM RDW
         CH    R9,=H'4'      AT LEAST MINIMUM ?
         BL    BADBLOCK      NO; BAD RECORD OR BAD BLOCK
         C     R9,ZPLRECL    VALID LENGTH ?                     GP14233
         BH    BADBLOCK      NO
         LA    R7,0(R9,R8)   SET ADDRESS OF LAST BYTE +1
         C     R7,BUFFEND    WILL IT FIT INTO BUFFER ?
         BL    DEBVCURR      LOW - LEAVE IT
         BH    BADBLOCK      NO; FAIL
         LA    R6,3          Preset for bad sdw                 GP14244
         SR    R7,R7         Preset for block done
DEBVCURR ST    R7,BUFFCURR        for recursion
         TM    3(R8),X'FF'   CLEAN RDW ?
         BNZ   BADBLOCK
         TM    IOPFLAGS,IOFLSDW   WAS PREVIOUS RECORD DONE ?
         BO    DEBVAPND           NO
         LH    R0,0(,R8)          Provisional length if simple
         ST    R0,0(,R4)          Return length
         ST    R0,KEPTREC+4       Remember record info
         CLI   2(R8),1            What is this?
         BL    SETCURR            Simple record
         BH    BADBLOCK           Not=1; have a sequence error
         OI    IOPFLAGS,IOFLSDW   Starting a new segment
         L     R2,VBSADDR         Get start of buffer
         MVC   0(4,R2),=X'00040000'   Preset null record
         B     DEBVMOVE           And move this
DEBVAPND CLI   2(R8),3            IS THIS A MIDDLE SEGMENT ?
         BE    DEBVMOVE           YES, PUT IT OUT
         CLI   2(R8),2            IS THIS THE LAST SEGMENT ?
         BNE   BADBLOCK           No; bad segment sequence
         NI    IOPFLAGS,255-IOFLSDW  INDICATE RECORD COMPLETE
DEBVMOVE L     R2,VBSADDR         Get segment assembly area
         SR    R1,R1              Never trust anyone
         ICM   R1,3,0(R8)         Length of addition
         SH    R1,=H'4'           Data length
         LA    R0,4(,R8)          Skip SDW
         SR    R15,R15
         ICM   R15,3,0(R2)        Get amount used so far
         LA    R14,0(R15,R2)      Address for next segment
         LA    R8,0(R1,R15)       New length
         STH   R8,0(,R2)          Update RDW
         A     R8,VBSADDR         New end address
         C     R8,VBSEND          Will it fit ?
         BH    BADBLOCK
         LR    R15,R1             Move all
         MVCL  R14,R0             Append segment
         TM    IOPFLAGS,IOFLSDW    Did last segment?
         BNZ   REREAD             No; get next one
         L     R8,VBSADDR         Give user the assembled record
         SR    R0,R0
         ICM   R0,3,0(R8)         Provisional length if simple
         ST    R0,0(,R4)          Return length
         ST    R0,KEPTREC+4       Remember record info
         B     SETCURR            Done
         SPACE 2
* If RECFM=FB, bump address by lrecl
*        R8 has address of current record
DEBLOCKF L     R7,ZPLRECL         Load RECFM=F DCB LRECL        GP14233
         ST    R7,0(,R4)          Return length
         ST    R7,KEPTREC+4       Remember record info
         AR    R7,R8              Find the next record address
* If address=BUFFEND, zero BUFFCURR
SETCURR  CL    R7,BUFFEND         Is it off end of block?
         BL    SETCURS            Is not off, go store it
         SR    R7,R7              Clear the next record address
SETCURS  ST    R7,BUFFCURR        Store the next record address
         ST    R8,0(,R3)          Store record address for caller
         ST    R8,KEPTREC         Remember record info
         B     READGOOD                                         GP14244
         SPACE 1
TGETREAD L     R6,ZIOECT          RESTORE ECT ADDRESS
         L     R7,ZIOUPT          RESTORE UPT ADDRESS
         MVI   ZGETLINE+2,X'80'   EXPECTED FLAG
         SAM24
         GETLINE PARM=ZGETLINE,ECT=(R6),UPT=(R7),ECB=ZIOECB,           *
               MF=(E,ZIOPL)
         SAM31
         LR    R6,R15             COPY RETURN CODE
         CH    R6,=H'16'          HIT BARRIER ?
         BE    READEOD2           YES; EOF, BUT ALLOW READS
         CH    R6,=H'8'           SERIOUS ?
         BNL   READEXNG           ATTENTION INTERRUPT OR WORSE
         L     R1,ZGETLINE+4      GET INPUT LINE
*---------------------------------------------------------------------*
*   MVS 3.8 undocumented behavior: at end of input in batch execution,
*   returns text of 'END' instead of return code 16. Needs DOC fix
*---------------------------------------------------------------------*
         CLC   =X'00070000C5D5C4',0(R1)  Undocumented EOF?
         BNE   TGETNEOF
         XC    KEPTREC(8),KEPTREC Clear saved record info
         LA    R6,1
         LNR   R6,R6              Signal EOF
         B     TGETFREE           FREE BUFFER AND QUIT
TGETNEOF L     R6,BUFFADDR        GET INPUT BUFFER
         LR    R8,R1              INPUT LINE W/RDW
         LH    R9,0(,R1)          GET LENGTH
         LR    R7,R9               FOR V, IN LEN = OUT LEN
         CLI   RECFMIX,IXVAR      RECFM=V ?
         BE    TGETHAVE           YES
         BL    TGETSKPF
         SH    R7,=H'4'           ALLOW FOR RDW
         B     TGETSKPV
TGETSKPF L     R7,ZPLRECL           FULL SIZE IF F              GP14233
TGETSKPV LA    R8,4(,R8)          SKIP RDW
         SH    R9,=H'4'           LENGTH SANS RDW
TGETHAVE ST    R6,0(,R3)          RETURN ADDRESS
         ST    R7,0(,R4)            AND LENGTH
         STM   R6,R7,KEPTREC      Remember record info
         ICM   R9,8,=C' '           BLANK FILL
         MVCL  R6,R8              PRESERVE IT FOR USER
         SR    R6,R6              NO EOF
TGETFREE LH    R0,0(,R1)          GET LENGTH
         ICM   R0,8,=AL1(1)       SUBPOOL 1
         FREEMAIN R,LV=(0),A=(1)  FREE SYSTEM BUFFER
READGOOD SR    R6,R6              Set good return               GP14244
         B     READEXIT           TAKE NORMAL EXIT
         SPACE 1
READEOD  OI    IOPFLAGS,IOFLEOF   Remember that we hit EOF
READEOD2 XC    KEPTREC(8),KEPTREC Clear saved record info
         NI    IOPFLAGS,255-IOFKEPT   Reset flag                GP14213
         LA    R6,1
READEXNG LNR   R6,R6              Signal EOF
READEXIT FUNEXIT RC=(R6)          Return to caller (0, 4, or -1)
         SPACE 1
*---------------------------------------------------------------------*
*   VSAM read
*---------------------------------------------------------------------*
VSAMREAD LA    R8,ZARPL      GET RPL ADDRESS                    GP14233
         LM    R5,R6,ZBUFF1  GET AVAILABLE BUFFER               GP14244
         MODCB RPL=(R8),AREA=(R5),AREALEN=(R6),OPTCD=(LOC),            *
               MF=(G,ZAMODCB)                                   GP14233
         GET   RPL=(R8)           Get a record                  GP14233
         TM    IOPFLAGS,IOFLEOF   EOF ?                         GP14244
         BNZ   READEOD              Yes; get out                GP14244
         BXH   R15,R15,EXRDBAD FAIL ON ERROR                    GP14233
*  N.B. I TRIED SHOWCB TO GET AREA & LENGTH, AND FAILED.        GP14244
         L     R5,RPLAREA-IFGRPL(,R8)  RECORD ADDRESS POINTER   GP14233
         L     R5,0(,R5)          GET RECORD ADDRESS
         L     R6,RPLRLEN-IFGRPL(,R8)  GET RECORD LENGTH        GP14233
         ST    R5,0(,R3)          Return the block address      GP14233
         ST    R6,0(,R4)            and length                  GP14233
         B     READGOOD                                         GP14233
         SPACE 1
         PUSH  USING                                            GP14244
         DROP  ,                                                GP14244
         USING VSAMEOD,R15                                      GP14244
         USING ZARPL,R1                                         GP14244
VSAMEOD  OI    IOPFLAGS,IOFLEOF   Set EOF flag                  GP14244
         BR    R14                  return to VSAM              GP14244
         POP   USING                                            GP14244
         SPACE 1
*---------------------------------------------------------------------*
*   VSAM LERAD AND SYNAD: SET ERROR CODE AND RETURN                   *
*---------------------------------------------------------------------*
VLERAD   DS    0H                                               GP14233
VSYNAD   LA    R15,8         DITTO                              GP14233
         BR    R14           RETURN TO VSAM                     GP14233
         SPACE 1
*---------------------------------------------------------------------*
*   VTOC read
*---------------------------------------------------------------------*
VTOCREAD CLC   ZVUSCCHH,ZVHICCHH  At or past VTOC end?          GP14213
         BNL   READEOD              Yes; quit with EOF          GP14213
         L     R14,PATSEEK        Get SEEK pattern              GP14213
         LA    R15,ZVUSCCHH       Requested address             GP14213
         LA    R0,ZVSER           Volume serial                 GP14213
         L     R1,ZBUFF1          Point to buffer               GP14213
         ST    R1,0(,R3)          Return the block address      GP14213
         LA    R2,DS1END-DS1DSNAM Length                        GP14213
         ST    R2,0(,R4)                                        GP14213
         STM   R14,R1,ZVSEEK      Complete CAMLST               GP14213
         SR    R14,R14            Clear for address increae     GP14213
         ICM   R14,3,ZVUSCCHH     Load cylinder                 GP14213
         SR    R15,R15                                          GP14213
         ICM   R15,3,ZVUSCCHH+2   Load track                    GP14213
         SR    R1,R1                                            GP14213
         IC    R1,ZVUSCCHH+L'ZVUSCCHH-1     Get current record  GP14213
         LA    R1,1(,R1)          Increase                      GP14213
         CLM   R1,1,ZVHIREC       Valid?                        GP14213
         BNH   VTOCSET@             Yes                         GP14213
         LA    R1,1               Set for new track record      GP14213
         LA    R15,1(,R15)        Space to new track            GP14213
         CH    R15,ZVTPCYL        Valid?                        GP14213
         BL    VTOCSET@             Yes                         GP14213
         SLR   R15,R15            Track on next cylinder        GP14213
         LA    R14,1(,R14)        New cylinder                  GP14213
         CLM   R14,3,ZVCPVOL      Valid?                        GP14213
         BNL   READEOD              No; fake EOF                GP14213
VTOCSET@ STH   R14,ZVUSCCHH                 New cylinder        GP14213
         STH   R15,ZVUSCCHH+2               New track           GP14213
         STC   R1,ZVUSCCHH+L'ZVUSCCHH-1     New record          GP14213
         CLC   ZVUSCCHH,ZVHICCHH  At or past VTOC end?          GP14213
         BNL   READEOD              Yes; quit with EOF          GP14213
         OBTAIN ZVSEEK            Read to VTOC block            GP14213
         BXLE  R15,R15,READGOOD   Normal return                 GP14213
         B     EXRDBAD            Abend                         GP14213
PATSEEK  CAMLST SEEK,*-*,*-*,*-*                                GP14213
         ORG   PATSEEK+4                                        GP14213
         SPACE 1
BADBLOCK LM    R14,R15,RSNLIST    GET START/SIZE OF BLOCK       GP14244
         AR    R15,R14            END + 1                       GP14244
         BCTR  R15,0              END                           GP14244
         ST    R15,RSNLIST+4      UPDATE END                    GP14244
         OI    RSNLIST+4,X'80'    SET END OF LIST               GP14244
         L     R15,=A(@@SNAP)                                   GP14244
         LA    R1,RSNAP                                         GP14244
         BALR  R14,R15            CALL SNAPPER                  GP14244
RSNDONE  WTO   'MVSSUPA - @@AREAD - problem processing RECFM=V(bs) file*
               ',ROUTCDE=11       Send to programmer and listing
         MVC   BADBLOT+8+13(8),ZDDN    Identify file            GP14244
         L     R1,=C'SRB?'                                      GP14244
         SLL   R6,3                                             GP14244
         SRL   R1,0(R6)                                         GP14244
         STC   R1,BADBLOT+8+32                                  GP14244
BADBLOT  WTO   'MVSSUPA - DD xxxxxxxx - INVALID xDW',                  *
               ROUTCDE=11         add more useful info          GP14244
         ABEND 1234,DUMP          Abend U1234 and allow a dump
RSNLIST  DC    A(0,0)             Snap list: Bad block          GP14244
RSNHEAD  DC    A(RSNHEAD1+X'80000000')  LABEL                   GP14244
RSNHEAD1 DC    AL1(RSNHEAD2-*-1),C'RECFM=Vxx bad block'         GP14244
RSNHEAD2 EQU   *                  END OF HEADERS                GP14244
RSNAP    SNAP  PDATA=(PSW,REGS),LIST=RSNLIST,STRHDR=RSNHEAD,MF=L
*
         LTORG ,                  In case someone adds literals
         SPACE 2
***********************************************************************
*                                                                     *
*  AWRITE - Write to an open data set                                 *
*                                                                     *
***********************************************************************
@@AWRITE FUNHEAD IO=YES,AM=YES,SAVE=SAVEADCB,US=NO   WRITE / PUT
         LR    R11,R1             SAVE PARM LIST
WRITMORE NI    IOPFLAGS,255-IOFCURSE   RESET RECURSION
         L     R4,4(,R11)         R4 points to the record address
         L     R4,0(,R4)          Get record address
         L     R5,8(,R11)         R5 points to length of data to write
         L     R5,0(,R5)          Length of data to write
         SPACE 1
WRITRITE SLR   R15,R15                                          GP14363
         IC    R15,IOIX           Branch by read type           GP14233
         B     *+4(R15)                                         GP14233
           B   WRITSAM              xSAM                        GP14233
           B   VSAMWRIT             VSAM                        GP14233
           B   6      ***VTOC write not supported***            GP14233
           B   TPUTWRIT             Terminal                    GP14233
*
WRITSAM  TM    IOMFLAGS,IOFBLOCK  Block mode?
         BNZ   WRITBLK            Yes
         CLI   OPENCLOS,X'84'     Running in update mode ?
         BNE   WRITENEW           No
         LM    R2,R3,KEPTREC      Get last record returned
         LTR   R3,R3              Any?
         BNP   WRITEEX            No; ignore (or abend?)
         CLI   RECFMIX,IXVAR      RECFM=V...
         BNE   WRITUPMV           NO
         LA    R0,4               ADJUST FOR RDW
         AR    R2,R0              KEEP OLD RDW
         SR    R3,R0              ADJUST REPLACE LENGTH
         AR    R4,R0              SKIP OVER USER'S RDW
         SR    R5,R0              ADJUST LENGTH
WRITUPMV MVCL  R2,R4              REPLACE DATA IN BUFFER
         OI    IOPFLAGS,IOFLDATA  SHOW DATA IN BUFFER
         B     WRITEEX            REWRITE ON NEXT READ OR CLOSE
         SPACE 1
WRITENEW CLI   RECFMIX,IXVAR      V-FORMAT ?
         BH    WRITBLK            U - WRITE BLOCK AS IS
         BL    WRITEFIX           F - ADD RECORD TO BLOCK
         TM    IOPFLAGS,IOFLSDW   CONTINUATION ?                GP14363
         BNZ   WRITESKC             YES; SKIP CHECK             GP14363
         CH    R5,0(,R4)          RDW LENGTH = REQUESTED LEN?
         BNE   WRITEBAD           NO; FAIL
WRITESKC L     R8,BUFFADDR        GET BUFFER                    GP14363
         ICM   R6,15,BUFFCURR     Get next record address
         BNZ   WRITEVAT
         LA    R0,4
         STH   R0,0(,R8)          BUILD BDW
         LA    R6,4(,R8)          SET TO FIRST RECORD POSITION
WRITEVAT L     R9,BUFFEND         GET BUFFER END
         SR    R9,R6              LESS CURRENT POSITION
         TM    ZRECFM,DCBRECSB    SPANNED?
         BZ    WRITEVAR           NO; ROUTINE VARIABLE WRITE
         LA    R1,4(,R5)          GET RECORD + BDW LENGTH
         C     R1,ZPLRECL         VALID SIZE?                   GP14233
         BH    WRITEBAD           NO; TAKE A DIVE
         TM    IOPFLAGS,IOFLSDW   CONTINUATION ?
         BNZ   WRITEVAW           YES; DO HERE
         CR    R5,R9              WILL IT FIT AS IS?
         BNH   WRITEVAS           YES; DON'T NEED TO SPLIT
WRITEVAW CH    R9,=H'5'           AT LEAST FIVE BYTES LEFT ?
         BL    WRITEVNU           NO; WRITE THIS BLOCK; RETRY
         LR    R3,R6              SAVE START ADDRESS
         LR    R7,R9              COPY LENGTH
         CR    R7,R5              ROOM FOR ENTIRE SEGMENT ?
         BL    *+4+2              NO
         LR    R7,R5              USE ONLY WHAT'S AVAILABLE
         MVCL  R6,R4              COPY SDW + DATA
         ST    R6,BUFFCURR        UPDATE NEXT AVAILABLE
         SR    R6,R8              LESS START
         STH   R6,0(,R8)          UPDATE BDW
         STH   R9,0(,R3)          FIX RDW LENGTH
         MVC   2(2,R3),=X'0100'   SET FLAGS FOR START SEGMENT
         OI    IOPFLAGS,IOFLDATA  SHOW WRITE DATA IN BUFFER     GP14363
         TM    IOPFLAGS,IOFLSDW   DID START ?
         BZ    WRITEWAY           NO; FIRST SEGMENT             GP14363
         MVI   2(R3),3            SHOW MIDDLE SEGMENT
         LTR   R5,R5              DID WE FINISH THE RECORD ?
         BP    WRITEWAY           NO
         MVI   2(R3),2            SHOW LAST SEGMENT
         NI    IOPFLAGS,255-IOFLSDW-IOFCURSE  RCD COMPLETE
         XC    KEPTREC(8),KEPTREC      Reset saved address/len  GP14363
         B     WRITEEX            DONE
WRITEWAY LA    R9,4               ALLOW FOR EXTRA RDW           GP14363
         SR    R4,R9                                            GP14363
         AR    R5,R9                                            GP14363
         STM   R4,R5,KEPTREC      MAKE FAKE PARM LIST
         OI    IOPFLAGS,IOFLSDW   SHOW PARTIAL RECORD IN BUFFER GP14363
         B     WRITPREP           GO FOR MORE                   GP14363
         SPACE 1
WRITEVAR LA    R1,4(,R5)          GET RECORD + BDW LENGTH
         C     R1,ZPBLKSZ         VALID SIZE?
         BH    WRITEBAD           NO; TAKE A DIVE
         L     R9,BUFFEND         GET BUFFER END
         SR    R9,R6              LESS CURRENT POSITION
         CR    R5,R9              WILL IT FIT ?
         BH    WRITEVNU           NO; WRITE NOW AND RECURSE
WRITEVAS LR    R7,R5              IN LENGTH = MOVE LENGTH
         MVCL  R6,R4              MOVE USER'S RECORD
         ST    R6,BUFFCURR        UPDATE NEXT AVAILABLE
         SR    R6,R8              LESS START
         STH   R6,0(,R8)          UPDATE BDW
         OI    IOPFLAGS,IOFLDATA  SHOW WRITE DATA IN BUFFER
         TM    DCBRECFM,DCBRECBR  BLOCKED?
         BNZ   WRITEEX              Yes; normal. Else write
         B     WRITPREP           Write it now
         SPACE 1
WRITEVNU OI    IOPFLAGS,IOFCURSE  SET RECURSION REQUEST
         B     WRITPREP           SET ADDRESS/LENGTH TO WRITE
         SPACE 1
WRITEBAD LA    R14,0(,R4)         Get current record address    GP14363
         LA    R15,0(,R5)           and length                  GP14363
         ST    R14,WSNLIST        Save address                  GP14363
         CH    R15,=H'32'         Only need first line?         GP14363
         BNH   *+8                                              GP14363
         LH    R15,=H'32'         so truncate it                GP14363
         AR    R15,R14            END + 1                       GP14251
         BCTR  R15,0              END                           GP14251
         ST    R15,WSNLIST+4      UPDATE END                    GP14251
         OI    WSNLIST+4,X'80'    SET END OF LIST               GP14251
         L     R15,=A(@@SNAP)                                   GP14251
         LA    R1,WSNAP                                         GP14251
         BALR  R14,R15            CALL SNAPPER                  GP14251
         WTO   'MVSSUPA - @@AWRITE - invalid RECFM=Vxx request',       *
               ROUTCDE=11       Send to programmer and listing  GP14251
         MVC   BADBLOW+8+13(8),ZDDN    Identify file            GP14251
BADBLOW  WTO   'MVSSUPA - DD xxxxxxxx',                                *
               ROUTCDE=11         add more useful info          GP14251
         ABEND 002,DUMP           INVALID REQUEST
WSNLIST  DC    A(0,0)             Snap list: Bad block          GP14251
WSNHEAD  DC    A(WSNHEAD1+X'80000000')  LABEL                   GP14251
WSNHEAD1 DC    AL1(WSNHEAD2-*-1),C'RECFM=Vxx bad record'        GP14363
WSNHEAD2 EQU   *                  END OF HEADERS                GP14251
WSNAP    SNAP  PDATA=(PSW,REGS),LIST=WSNLIST,STRHDR=WSNHEAD,MF=L
         SPACE 1
WRITEFIX ICM   R6,15,BUFFCURR     Get next available record
         BNZ   WRITEFAP           Not first
         L     R6,BUFFADDR        Get buffer start
WRITEFAP L     R7,ZPLRECL         Record length                 GP14233
         ICM   R5,8,=C' '         Request blank padding
         MVCL  R6,R4              Copy record to buffer
         ST    R6,BUFFCURR        Update new record address
         OI    IOPFLAGS,IOFLDATA  SHOW DATA IN BUFFER
         TM    DCBRECFM,DCBRECBR  BLOCKED?                      GP14363
         BZ    WRITPREP             No; write it now            GP14363
         C     R6,BUFFEND         Room for more ?
         BL    WRITEEX            YES; RETURN
WRITPREP L     R4,BUFFADDR        Start write address
         LR    R5,R6              Current end of block
         SR    R5,R4              Current length
*NEXT*   B     WRITBLK            WRITE THE BLOCK
         SPACE 1
WRITBLK  AR    R5,R4              Set start and end of write
         STM   R4,R5,BUFFADDR     Pass to physical writer
         OI    IOPFLAGS,IOFLDATA  SHOW DATA IN BUFFER
         FIXWRITE ,               Write physical block
         TM    IOPFLAGS,IOFLSDW   Partial record ?              GP14363
         BZ    WRITEEX              No; just return             GP14363
         LM    R4,R5,KEPTREC      Residual text & length        GP14363
         B     WRITENEW             Finish record               GP14363
VSAMWRIT LA    R8,ZARPL      GET RPL ADDRESS                    GP14233
         L     R5,ZBUFF1     GET BUFFER                         GP14233
         MODCB RPL=(R8),AREA=(R4),AREALEN=(R5),OPTCD=(MVE),            *
               MF=(G,ZAMODCB)                                   GP14233
         PUT   RPL=(R8)           Get a record                  GP14233
         BXH   R15,R15,WRITBAD FAIL ON ERROR                    GP14233
         CHECK RPL=(R8)      TAKE APPLICABLE EXITS              GP14233
         BXLE  R15,R15,WRITEEX    Return                        GP14233
WRITBAD  ABEND 001,DUMP           Or set error code?            GP14233
         SPACE 1
TPUTWRIT CLI   RECFMIX,IXVAR      RECFM=V ?
         BE    TPUTWRIV           YES
         SH    R4,=H'4'           BACK UP TO RDW
         LA    R5,4(,R5)          LENGTH WITH RDW
TPUTWRIV STH   R5,0(,R4)          FILL RDW
         STCM  R5,12,2(R4)          ZERO REST
         L     R6,ZIOECT          RESTORE ECT ADDRESS
         L     R7,ZIOUPT          RESTORE UPT ADDRESS
         SAM24
         PUTLINE PARM=ZPUTLINE,ECT=(R6),UPT=(R7),ECB=ZIOECB,           *
               OUTPUT=((R4),DATA),TERMPUT=EDIT,MF=(E,ZIOPL)
         SAM31
         SPACE 1
WRITEEX  TM    IOPFLAGS,IOFCURSE  RECURSION REQUESTED?
         BNZ   WRITMORE           PROCESS REMAINING DATA        GP14363
         FUNEXIT RC=0             EXIT
         SPACE 2
***********************************************************************
*                                                                     *
*  ANOTE  - Remember the position in the data set (BSAM/BPAM only)    *
*                                                                     *
***********************************************************************
@@ANOTE  FUNHEAD IO=YES,AM=YES,SAVE=SAVEADCB,US=NO   NOTE position
         L     R3,4(,R1)          R3 points to the return value
         FIXWRITE ,
         SAM24 ,                  For old code
         TM    IOMFLAGS,IOFEXCP   EXCP mode?
         BZ    NOTEBSAM           No
         L     R4,DCBBLKCT        Return block count
         B     NOTECOM
         SPACE 1
NOTEBSAM NOTE  (R10)              Note current position
         LR    R4,R1              Save result
NOTECOM  AMUSE ,
         ST    R4,0(,R3)          Return TTR0 to user
         FUNEXIT RC=0
         SPACE 2
***********************************************************************
*                                                                     *
*  APOINT - Restore the position in the data set (BSAM/BPAM only)     *
*           Note that this does not fail; it just bombs on the        *
*           next read or write if incorrect.                          *
*                                                                     *
***********************************************************************
@@APOINT FUNHEAD IO=YES,AM=YES,SAVE=SAVEADCB,US=NO   NOTE position
         L     R3,4(,R1)          R3 points to the TTR value
         L     R3,0(,R3)          Get the TTR
         ST    R3,ZWORK           Save below the line
         FIXWRITE ,
         SAM24 ,                  For old code
         TM    IOMFLAGS,IOFEXCP   EXCP mode ?
         BZ    POINBSAM           No
         L     R4,DCBBLKCT        Get current position
         SR    R4,R3              Get new position's increment
         BZ    POINCOM
         BM    POINHEAD
POINBACK MVI   TAPECCW,X'27'      Backspace
         B     POINECOM
POINHEAD MVI   TAPECCW,X'37'      Forward space
POINECOM LA    R0,1
         STH   R0,TAPECCW+6
         LPR   R4,R4
POINELUP EXCP  TAPEIOB
         WAIT  ECB=TAPEECB
         BCT   R4,POINELUP
         ST    R3,DCBBLKCT
         B     POINCOM
         SPACE 1
POINBSAM POINT (R10),ZWORK        Request repositioning
POINCOM  AMUSE ,
         NI    IOPFLAGS,255-IOFLEOF   Valid POINT resets EOF
         XC    KEPTREC(8),KEPTREC      Also clear record data
         FUNEXIT RC=0
         SPACE 2
***********************************************************************
*                                                                     *
*  ADCBA - Report the DCB parameters for an open file.                *
*                                                                     *
***********************************************************************
@@ADCBA  FUNHEAD IO=YES,AM=YES,SAVE=SAVEADCB,US=NO   READ / GET GP14205
         L     R3,4(,R1)   R3 points to where to store the IOMODE
         L     R4,8(,R1)   R3 points to where to store record format
         L     R5,12(,R1)  R4 points to where to store record length
         L     R6,16(,R1)  R5 points to where to store block size
         MVC   0(4,R3),ZDVTYPE    DevTyp,RecFm,IOS+IOM flags    GP14251
         SR    R0,R0                                            GP14205
         IC    R0,RECFMIX                                       GP14233
         SRL   R0,2               Change to flag                GP14233
         ST    R0,0(,R4)          Return RECFM                  GP14205
         L     R0,ZPLRECL                                       GP14205
         ST    R0,0(,R5)          Return LRECL                  GP14205
         L     R0,ZPBLKSZ                                       GP14205
         ST    R0,0(,R6)          Return BLKSIZE                GP14205
         FUNEXIT RC=0             Return to caller              GP14205
         SPACE 2
***********************************************************************
*                                                                     *
*  ACLOSE - Close a data set                                          *
*                                                                     *
***********************************************************************
@@ACLOSE FUNHEAD IO=YES,SAVE=(WORKAREA,WORKLEN,SUBPOOL)  CLOSE
         TM    IOMFLAGS,IOFTERM   TERMINAL I/O MODE?
         BNZ   FREEBUFF           YES; JUST FREE STUFF
         FIXWRITE ,          WRITE FINAL BUFFER, IF ONE
FREEBUFF LM    R1,R2,ZBUFF1       Look at first buffer
         LTR   R0,R2              Any ?
         BZ    FREEDBF1           No
         FREEMAIN RC,LV=(0),A=(1),SP=SUBPOOL  Free BLOCK buffer
FREEDBF1 LM    R1,R2,ZBUFF2       Look at second buffer
         LTR   R0,R2              Any ?
         BZ    FREEDBF2           No
         FREEMAIN RC,LV=(0),A=(1),SP=SUBPOOL  Free RECRD buffer
FREEDBF2 TM    IOMFLAGS,IOFTERM   TERMINAL I/O MODE?
         BNZ   NOPOOL             YES; SKIP CLOSE/FREEPOOL
         CLOSE MF=(E,OPENCLOS)
         TM    DCBBUFCA+L'DCBBUFCA-1,1      BUFFER POOL?
         BNZ   NOPOOL             NO, INVALIDATED
         SR    R15,R15
         ICM   R15,7,DCBBUFCA     DID WE GET A BUFFER?
         BZ    NOPOOL             0-NO
         FREEPOOL ((R10))
NOPOOL   DS    0H
         FREEMAIN R,LV=ZDCBLEN,A=(R10),SP=SUBPOOL
         FUNEXIT RC=0
         SPACE 2
         PUSH  USING
         DROP  ,
*---------------------------------------------------------------------*
*  Physical Write - called by @@ACLOSE, switch from output to input
*    mode, and whenever output buffer is full or needs to be emptied.
*  Works for EXCP and BSAM. Special processing for UPDAT mode
*---------------------------------------------------------------------*
TRUNCOUT B     *+14-TRUNCOUT(,R15)   SKIP LABEL
         DC    AL1(9),CL(9)'TRUNCOUT' EXPAND LABEL
         AIF   ('&ZSYS' NE 'S380').NOTRUBS
         BSM   R14,R0             PRESERVE AMODE
.NOTRUBS STM   R14,R12,12(R13)    SAVE CALLER'S REGISTERS
         LR    R12,R15
         USING TRUNCOUT,R12
         LA    R15,ZIOSAVE2-ZDCBAREA(,R10)
         ST    R15,8(,R13)
         ST    R13,4(,R15)
         LR    R13,R15
         USING IHADCB,R10    COMMON I/O AREA SET BY CALLER
         TM    IOPFLAGS,IOFLDATA   PENDING WRITE ?
         BZ    TRUNCOEX      NO; JUST RETURN
         SAM24 ,             GET LOW
         LM    R4,R5,BUFFADDR  START/NEXT ADDRESS
         CLI   RECFMIX,IXVAR      RECFM=V?
         BNE   TRUNLEN5
         LH    R5,0(,R4)     Get BDW length field               GP14363
         CL    R5,=F'8'      Empty or invalid ?                 GP14363
         BNH   TRUNPOST        Yes; ignore and reset buffer     GP14363
         B     TRUNTMOD           CHECK OUTPUT TYPE
TRUNLEN5 SR    R5,R4              CONVERT TO LENGTH
         BNP   TRUNPOST           NOTHING TO DO                 GP14363
TRUNTMOD DS    0H
         TM    IOMFLAGS,IOFEXCP   EXCP mode ?
         BNZ   EXCPWRIT           Yes
         CLI   OPENCLOS,X'84'     Update mode?
         BE    TRUNSHRT             Yes; just rewrite as is
         CLI   RECFMIX,IXVAR      RECFM=F ?
         BNL   *+8                No; leave it alone
         STH   R5,DCBBLKSI        Why do I need this?
         WRITE DECB,SF,(R10),(R4),(R5),MF=E  Write block
         B     TRUNCHK
TRUNSHRT WRITE DECB,SF,MF=E       Rewrite block from READ
TRUNCHK  CHECK DECB
         B     TRUNPOST           Clean up
         SPACE 1
EXCPWRIT STH   R5,TAPECCW+6
         STCM  R4,7,TAPECCW+1     WRITE FROM TEXT
         NI    DCBIFLGS,255-DCBIFEC   ENABLE ERP
         OI    DCBIFLGS,X'40'     SUPPRESS DDR
         STCM  R5,12,IOBSENS0-IOBSTDRD+TAPEIOB   CLEAR SENSE
         OI    DCBOFLGS-IHADCB+TAPEDCB,DCBOFLWR  SHOW WRITE
         XC    TAPEECB,TAPEECB
         EXCP  TAPEIOB
         WAIT  ECB=TAPEECB
         TM    TAPEECB,X'7F'      GOOD COMPLETION?
         BO    TRUNPOST
*NEXT*   BNO   EXWRN7F            NO
         SPACE 1
EXWRN7F  TM    IOBUSTAT-IOBSTDRD+TAPEIOB,IOBUSB7  END OF TAPE?
         BNZ   EXWREND       YES; SWITCH TAPES
         CLC   =X'1020',IOBSENS0-IOBSTDRD+TAPEIOB  EXCEEDED AWS/HET ?
         BNE   EXWRB001
EXWREND  L     R15,DCBBLKCT
         SH    R15,=H'1'
         ST    R15,DCBBLKCT       ALLOW FOR EOF 'RECORD'
         EOV   TAPEDCB       TRY TO RECOVER
         B     EXCPWRIT
         SPACE 1
EXWRB001 LA    R9,TAPEIOB    GET IOB FOR QUICK REFERENCE
         ABEND 001,DUMP
         SPACE 1
TRUNPOST XC    BUFFCURR,BUFFCURR  CLEAR
         NI    IOPFLAGS,255-IOFLDATA  Reset it                  GP14363
         CLI   RECFMIX,IXVAR      RECFM=V
         BL    TRUNCOEX           F - JUST EXIT
         LA    R4,4               BUILD BDW
         L     R3,BUFFADDR        GET BUFFER
         STH   R4,0(,R3)          UPDATE
         LA    R4,0(R4,R3)                                      GP14363
         ST    R4,BUFFCURR        SET NEXT AVAILABLE            GP14363
TRUNCOEX L     R13,4(,R13)
         LM    R14,R12,12(R13)    Reload all
         QBSM  0,R14              Return in caller's mode
         LTORG ,
         POP   USING
         SPACE 2
***********************************************************************
*                                                                     *
*  GETM - GET MEMORY                                                  *
*                                                                     *
***********************************************************************
@@GETM   FUNHEAD ,
*
         LDADD R3,0(,R1)          LOAD REQUESTED STORAGE SIZE
         SLR   R1,R1              PRESET IN CASE OF ERROR
         LTR   R4,R3              CHECK REQUEST
         BNP   GETMEX             QUIT IF INVALID
*
* To reduce fragmentation, round up size to 64 byte multiple
*
         A     R3,=A(8+(64-1))    OVERHEAD PLUS ROUNDING
         N     R3,=X'FFFFFFC0'    MULTIPLE OF 64
*
         AIF   ('&ZSYS' EQ 'S370').NOANY                        GP14233
         GETMAIN RU,LV=(R3),SP=SUBPOOL,LOC=ANY
         AGO   .FINANY
.NOANY   ANOP  ,
         GETMAIN RU,LV=(R3),SP=SUBPOOL
.FINANY  ANOP  ,
*
* WE STORE THE AMOUNT WE REQUESTED FROM MVS INTO THIS ADDRESS
         ST    R3,0(,R1)
* AND JUST BELOW THE VALUE WE RETURN TO THE CALLER, WE SAVE
* THE AMOUNT THEY REQUESTED
         ST    R4,4(,R1)
         A     R1,=F'8'
*
GETMEX   FUNEXIT RC=(R1)
         LTORG ,
         SPACE 2
***********************************************************************
*                                                                     *
*  FREEM - FREE MEMORY                                                *
*                                                                     *
***********************************************************************
@@FREEM  FUNHEAD ,
*
         L     R1,0(,R1)
         S     R1,=F'8'
         L     R0,0(,R1)
*
         FREEMAIN RC,LV=(0),A=(1),SP=SUBPOOL
*
         FUNEXIT RC=(15)
         LTORG ,
         SPACE 2
***********************************************************************
*                                                                     *
*  GETCLCK - GET THE VALUE OF THE MVS CLOCK TIMER AND MOVE IT TO AN   *
*  8-BYTE FIELD.  THIS 8-BYTE FIELD DOES NOT NEED TO BE ALIGNED IN    *
*  ANY PARTICULAR WAY.                                                *
*                                                                     *
*  E.G. CALL 'GETCLCK' USING WS-CLOCK1                                *
*                                                                     *
*  THIS FUNCTION ALSO RETURNS THE NUMBER OF SECONDS SINCE 1970-01-01  *
*  BY USING SOME EMPIRICALLY-DERIVED MAGIC NUMBERS                    *
*                                                                     *
***********************************************************************
@@GETCLK FUNHEAD ,                GET TOD CLOCK VALUE
*
         L     R2,0(,R1)
         STCK  0(R2)
         L     R4,0(,R2)
         L     R5,4(,R2)
         SRDL  R4,12
         SL    R4,=X'0007D910'
         D     R4,=F'1000000'
         SL    R5,=F'1220'
*
RETURNGC FUNEXIT RC=(R5)
         LTORG ,
         SPACE 2
***********************************************************************
*                                                                     *
*  GETTZ - Get the offset from GMT in 1.048576 seconds                *
*                                                                     *
***********************************************************************
* @@GETTZ FUNHEAD ,                 get timezone offset
*
*         L     R3,CVTPTR
*         USING CVT,R3
*         L     R4,CVTTZ
*
* RETURNGS FUNEXIT RC=(R4)
*         LTORG ,
         ENTRY @@GETTZ
@@GETTZ  L     R15,CVTPTR
         L     R15,CVTTZ-CVTMAP(,R15)  GET GMT TIME-ZONE OFFSET
         BR    R14
         SPACE 2
***********************************************************************
*                                                                     *
*    CALL @@SYSTEM,(req-type,pgm-len,pgm-name,parm-len,parm),VL       *
*                                                                     *
*    "-len" fields are self-defining values in the calling list,      *
*        or else pointers to 32-bit signed integer values             *
*                                                                     *
*    "pgm-name" is the address of the name of the program to be       *
*        executed (one to eight characters)                           *
*                                                                     *
*    "parm" is the address of a text string of length "parm-len",     *
*        and may be zero to one hundred bytes (OS JCL limit)          *
*                                                                     *
*    "req-type" is or points to 1 for a program ATTACH                *
*                               2 for TSO CP invocation               *
*                                                                     *
*---------------------------------------------------------------------*
*                                                                     *
*     Author:  Gerhard Postpischil                                    *
*                                                                     *
*     This code is placed in the public domain.                       *
*                                                                     *
*---------------------------------------------------------------------*
*                                                                     *
*     Assembly: Any MVS or later assembler may be used.               *
*        Requires SYS1.MACLIB. TSO CP support requires additional     *
*        macros from SYS1.MODGEN (SYS1.AMODGEN in MVS).               *
*        Intended to work in any 24 and 31-bit environment.           *
*                                                                     *
*     Linker/Binder: RENT,REFR,REUS                                   *
*                                                                     *
*---------------------------------------------------------------------*
*     Return codes:  when R15:0 R15:1-3 has return from program.      *
*       R15 is 04806nnn  ATTACH failed                                *
*       R15 is 1400000n  PARM list error: n= 1,2, or 3 (req/pgm/parm) *
*       R15 is 80sss000 or 80000uuu Subtask ABENDED (SYS sss/User uuu)*
*                                                                     *
***********************************************************************
@@SYSTEM FUNHEAD SAVE=(SYSATWRK,SYSATDLN,78)  ISSUE OS OR TSO COMMAND
         L     R15,4(,R13)        GET CALLER'S SAVE AREA
         LA    R11,16(,R15)       REMEMBER THE RETURN CODE ADDRESS
         LR    R9,R1              SAVE PARAMETER LIST ADDRESS
         SPACE 1
         MVC   0(4,R11),=X'14000002'  PRESET FOR PARM ERROR
         LDADD R4,0(,R9)          REQUEST TYPE
         LDADD R5,4(,R9)          LENGTH OF PROGRAM NAME
         L     R6,8(,R9)          -> PROGRAM NAME
         LDADD R7,12(,R9)         LENGTH OF PARM
         L     R8,16(,R9)         -> PARM TEXT
         SPACE 1
*   NOTE THAT THE CALLER IS EITHER COMPILER CODE, OR A COMPILER
*   LIBRARY ROUTINE, SO WE DO MINIMAL VALIDITY CHECKING
*
*   EXAMINE PROGRAM NAME LENGTH AND STRING
*
         CH    R5,=H'8'           NOT TOO LONG ?
         BH    SYSATEXT           TOO LONG; TOO BAD
         SH    R5,=H'1'           LENGTH FOR EXECUTE
         BM    SYSATEXT           NONE; OOPS
         MVC   SYSATPGM(L'SYSATPGM+L'SYSATOTL+1),=CL11' '  PRE-BLANK
         EX    R5,SYSAXPGM        MOVE PROGRAM NAME
         CLC   SYSATPGM,=CL11' '  STILL BLANK ?
         BE    SYSATEXT           YES; TOO BAD
*   BRANCH AND PROCESS ACCORDING TO REQUEST TYPE
*
         MVI   3(R11),1           SET BAD REQUEST TYPE
         CH    R4,=H'2'           CP PROGRAM ATTACH ?
         BE    SYSATCP            YES
         CH    R4,=H'1'           OS PROGRAM ATTACH ?
         BNE   SYSATEXT           NO; HAVE ERROR CODE
*   OS PROGRAM ATTACH - PREPARE PARM, ETC.
*
*   NOW LOOK AT PARM STRING
         LTR   R7,R7              ANY LENGTH ?
         BM    SYSATEXT           NO; OOPS
         STH   R7,SYSATOTL        PASS LENGTH OF TEXT
         BZ    SYSATNTX
         CH    R7,=AL2(L'SYSATOTX)  NOT TOO LONG ?
         BH    SYSATEXT           TOO LONG; TOO BAD
         BCTR  R7,0
         EX    R7,SYSAXTXT        MOVE PARM STRING
SYSATNTX LA    R1,SYSATOTL        GET PARAMETER ADDRESS
         ST    R1,SYSATPRM        SET IT
         OI    SYSATPRM,X'80'     SET END OF LIST BIT
         B     SYSATCOM           GO TO COMMON ATTACH ROUTINE
*   TSO CP REQUEST - PREPARE PARM, CPPL, ETC.
*
SYSATCP  LTR   R7,R7              ANY LENGTH ?
         BM    SYSATEXT           NO; OOPS
         LA    R1,SYSATOTX-SYSATOPL(,R7)  LENGTH WITH HEADER
         STH   R1,SYSATOPL        PASS LENGTH OF COMMAND TEXT
         LA    R1,1(,R5)          BYTE AFTER COMMAND NAME
         STH   R1,SYSATOPL+2      LENGTH PROCESSED BY PARSER
         BZ    SYSATXNO
         CH    R7,=AL2(L'SYSATOTX)  NOT TOO LONG ?
         BH    SYSATEXT           TOO LONG; TOO BAD
         BCTR  R7,0
         EX    R7,SYSAXTXT        MOVE PARM STRING
SYSATXNO LA    R1,SYSATOPL        GET PARAMETER ADDRESS
         ST    R1,SYSATPRM        SET IT
*   TO MAKE THIS WORK, WE NEED THE UPT, PSCB, AND ECT ADDRESS.
*   THE FOLLOWING CODE WORKS PROVIDED THE CALLER WAS INVOKED AS A
*   TSO CP, USED NORMAL SAVE AREA CONVENTIONS, AND HASN'T MESSED WITH
*   THE TOP SAVE AREA.
         MVI   3(R11),4           SET ERROR FOR BAD CP REQUEST
         LA    R2,SYSATPRM+8      CPPLPSCB
         EXTRACT (R2),FIELDS=PSB  GET THE PSCB
         PUSH  USING
         L     R1,PSATOLD-PSA     GET THE CURRENT TCB
         USING TCB,R1
         L     R1,TCBFSA          GET THE TOP LEVEL SAVE AREA
         N     R1,=X'00FFFFFF'    KILL TCBIDF BYTE
         POP   USING
         L     R1,24(,R1)         ORIGINAL R1
         LA    R1,0(,R1)            CLEAN IT
         LTR   R1,R1              ANY?
         BZ    SYSATEXT           NO; TOO BAD
         TM    0(R1),X'80'        END OF LIST?
         BNZ   SYSATEXT           YES; NOT CPPL
         TM    4(R1),X'80'        END OF LIST?
         BNZ   SYSATEXT           YES; NOT CPPL
         TM    8(R1),X'80'        END OF LIST?
         BNZ   SYSATEXT           YES; NOT CPPL
         CLC   8(4,R1),SYSATPRM+8   MATCHES PSCB FROM EXTRACT?
         BNE   SYSATEXT           NO; TOO BAD
         MVC   SYSATPRM+4(3*4),4(R1)  COPY UPT, PSCB, ECT
         L     R1,12(,R1)
         LA    R1,0(,R1)     CLEAR EOL BIT IN EITHER AMODE
         LTR   R1,R1         ANY ADDRESS?
         BZ    SYSATCOM      NO; SKIP
         PUSH  USING         (FOR LATER ADDITIONS?)
         USING ECT,R1        DECLARE ECT
         LM    R14,R15,SYSATPGM   GET COMMAND NAME
         LA    R0,7          MAX TEST/SHIFT
SYSATLCM CLM   R14,8,=CL11' '  LEADING BLANK ?
         BNE   SYSATLSV      NO; SET COMMAND NAME
         SLDL  R14,8         ELIMINATE LEADING BLANK
         IC    R15,=CL11' '  REPLACE BY TRAILING BLANK
         BCT   R0,SYSATLCM   TRY AGAIN
SYSATLSV STM   R14,R15,ECTPCMD
         NI    ECTSWS,255-ECTNOPD      SET FOR OPERANDS EXIST
         EX    R7,SYSAXBLK   SEE IF ANY OPERANDS
         BNE   SYSATCOM           HAVE SOMETHING
         OI    ECTSWS,ECTNOPD     ALL BLANK
         POP   USING
SYSATCOM LA    R1,SYSATPRM        PASS ADDRESS OF PARM ADDRESS
         LA    R2,SYSATPGM        POINT TO NAME
         LA    R3,SYSATECB        AND ECB
         ATTACH EPLOC=(R2),       INVOKE THE REQUESTED PROGRAM         *
               ECB=(R3),SF=(E,SYSATLST)  SZERO=NO,SHSPV=78
         LTR   R15,R15            CHECK RETURN CODE
         BZ    SYSATWET           GOOD
         MVC   0(4,R11),=X'04806000'  ATTACH FAILED
         STC   R15,3(,R11)        SET ERROR CODE
         B     SYSATEXT           FAIL
SYSATWET ST    R1,SYSATTCB        SAVE FOR DETACH
         WAIT  ECB=SYSATECB       WAIT FOR IT TO FINISH
         L     R2,SYSATTCB        GET SUBTASK TCB
         USING TCB,R2             DECLARE IT
         MVC   0(4,R11),TCBCMP    COPY RETURN OR ABEND CODE
         TM    TCBFLGS,TCBFA      ABENDED ?
         BZ    *+8                NO
         MVI   0(R11),X'80'       SET ABEND FLAG
         DETACH SYSATTCB          GET RID OF SUBTASK
         DROP  R2
         B     SYSATEXT           AND RETURN
SYSAXPGM OC    SYSATPGM(0),0(R6)  MOVE NAME AND UPPER CASE
SYSAXTXT MVC   SYSATOTX(0),0(R8)    MOVE PARM TEXT
SYSAXBLK CLC   SYSATOTX(0),SYSATOTX-1  TEST FOR OPERANDS
*    PROGRAM EXIT, WITH APPROPRIATE RETURN CODES
*
SYSATEXT FUNEXIT ,           RESTORE REGS; SET RETURN CODES
         SPACE 1             RETURN TO CALLER
*    DYNAMICALLY ACQUIRED STORAGE
*
SYSATWRK DSECT ,             MAP STORAGE
         DS    18A           OUR OS SAVE AREA
SYSATCLR DS    0F            START OF CLEARED AREA
SYSATLST ATTACH EPLOC=SYSATPGM,ECB=SYSATECB,SHSPV=78,SZERO=NO,SF=L
SYSATECB DS    F             EVENT CONTROL FOR SUBTASK
SYSATTCB DS    A             ATTACH TOKEN FOR CLEAN-UP
SYSATPRM DS    4A            PREFIX FOR CP
SYSATOPL DS    2Y     1/4    PARM LENGTH / LENGTH SCANNED
SYSATPGM DS    CL8    2/4    PROGRAM NAME (SEPARATOR)
SYSATOTL DS    Y      3/4    OS PARM LENGTH / BLANKS FOR CP CALL
SYSATZER EQU   SYSATCLR,*-SYSATCLR,C'X'   ADDRESS & SIZE TO CLEAR
SYSATOTX DS    CL247  4/4    NORMAL PARM TEXT STRING
SYSATDLN EQU   *-SYSATWRK     LENGTH OF DYNAMIC STORAGE
MVSSUPA  CSECT ,             RESTORE
         SPACE 2
***********************************************************************
*                                                                     *
*    INVOKE IDCAMS: CALL @@IDCAMS,(@LEN,@TEXT)                        *
*                                                                     *
***********************************************************************
         PUSH  USING
         DROP  ,
@@IDCAMS FUNHEAD SAVE=IDCSAVE     EXECUTE IDCAMS REQUEST
         LA    R1,0(,R1)          ADDRESS OF IDCAMS REQUEST (V-CON)
         ST    R1,IDC@REQ         SAVE REQUEST ADDRESS
         MVI   EXFLAGS,0          INITIALIZE FLAGS
         LA    R1,AMSPARM         PASS PARAMETER LIST
         LINK  EP=IDCAMS          INVOKE UTILITY
         FUNEXIT RC=(15)          RESTORE CALLER'S REGS
         POP   USING
         SPACE 1
***********************************************************************
*                                                                     *
* XIDCAMS - ASYNCHRONOUS EXIT ROUTINE                                 *
*                                                                     *
***********************************************************************
         PUSH  USING
         DROP  ,
XIDCAMS  STM   R14,R12,12(R13)
         LR    R12,R15
         USING XIDCAMS,R12
         LA    R9,XIDSAVE         SET MY SAVE AREA
         ST    R13,4(,R9)         MAKE BACK LINK
         ST    R9,8(,R13)         MAKE DOWN LINK
         LR    R13,R9             MAKE ACTIVE SAVE AREA
         SR    R15,R15            PRESET FOR GOOD RETURN
         LM    R3,R5,0(R1)        LOAD PARM LIST ADDRESSES
         SLR   R14,R14
         IC    R14,0(,R4)         LOAD FUNCTION
         B     *+4(R14)
         B     XIDCEXIT   OPEN           CODE IN R14 = X'00'
         B     XIDCEXIT   CLOSE          CODE IN R14 = X'04'
         B     XIDCGET    GET SYSIN      CODE IN R14 = X'08'
         B     XIDCPUT    PUT SYSPRINT   CODE IN R14 = X'0C'
XIDCGET  TM    EXFLAGS,EXFGET            X'01' = PRIOR GET ISSUED ?
         BNZ   XIDCGET4                  YES, SET RET CODE = 04
         L     R1,IDC@REQ         GET REQUEST ADDRESS
         LDADD R3,0(,R1)          LOAD LENGTH
         L     R2,4(,R1)          LOAD TEXT POINTER
         LA    R2,0(,R2)          CLEAR HIGH
         STM   R2,R3,0(R5)        PLACE INTO IDCAMS LIST
         OI    EXFLAGS,EXFGET            X'01' = A GET HAS BEEN ISSUED
         B     XIDCEXIT
XIDCGET4 LA    R15,4                     SET REG 15 = X'00000004'
         B     XIDCEXIT
XIDCPUT  TM    EXFLAGS,EXFSUPP+EXFSKIP  ANY FORM OF SUPPRESSION?
         BNZ   XIDCPUTZ           YES; DON'T BOTHER WITH REST
         LM    R4,R5,0(R5)
         LA    R4,1(,R4)          SKIP CARRIAGE CONTROL CHARACTER
         BCTR  R5,0               FIX LENGTH
         ICM   R5,8,=C' '         BLANK FILL
         LA    R14,XIDCTEXT
         LA    R15,L'XIDCTEXT
         MVCL  R14,R4
         TM    EXFLAGS,EXFMALL    PRINT ALL MESSAGES?
         BNZ   XIDCSHOW           YES; PUT THEM ALL OUT
         CLC   =C'IDCAMS ',XIDCTEXT    IDCAMS TITLE ?
         BE    XIDCEXIT           YES; SKIP
         CLC   XIDCTEXT+1(L'XIDCTEXT-1),XIDCTEXT   ALL BLANK OR SOME?
         BE    XIDCEXIT           YES; SKIP
         CLC   =C'IDC0002I',XIDCTEXT   AMS PGM END
         BE    XIDCEXIT           YES; SKIP
XIDCSHOW DS    0H            Consider how/whether to pass to user
*later   WTO   MF=(E,AMSPRINT)
XIDCPUTZ SR    R15,R15
         B     XIDCEXIT
XIDCSKIP OI    EXFLAGS,EXFSKIP    SKIP THIS AND REMAINING MESSAGES
         SR    R15,R15
*---------------------------------------------------------------------*
* IDCAMS ASYNC EXIT ROUTINE - EXIT, CONSTANTS & WORKAREAS
*---------------------------------------------------------------------*
XIDCEXIT L     R13,4(,R13)        GET CALLER'S SAVE AREA
         L     R14,12(,R13)
         RETURN (0,12)            RESTORE AND RETURN TO IDCAMS
IDCSAVE  DC    18F'0'             MAIN ROUTINE'S REG SAVEAREA
XIDSAVE  DC    18F'0'             ASYNC ROUTINE'S REG SAVEAREA
AMSPRINT DC    0A(0),AL2(4+L'XIDCTEXT,0)
XIDCTEXT DC    CL132' '
AMSPARM  DC    A(HALF00,HALF00,HALF00,X'80000000'+ADDRLIST)
ADDRLIST DC    F'2'
         DC    A(DDNAME01)
         DC    A(XIDCAMS)
IDC@REQ  DC    A(0)               ADDRESS OF REQUEST POINTER
         DC    A(DDNAME02)
         DC    A(XIDCAMS)
         DC    A(0)
HALF00   DC    H'0'
DDNAME01 DC    CL10'DDSYSIN   '
DDNAME02 DC    CL10'DDSYSPRINT'
EXFLAGS  DC    X'08'              EXIT PROCESSING FLAGS
EXFGET   EQU   X'01'                PRIOR GET WAS ISSUED
EXFNOM   EQU   X'04'                SUPPRESS ERROR WTOS
EXFRET   EQU   X'08'                NO ABEND; RETURN WITH COND.CODE
EXFMALL  EQU   X'10'                ALWAYS PRINT MESSAGES
EXFSUPP  EQU   X'20'                ALWAYS SUPPRESS MESSAGES
EXFSKIP  EQU   X'40'                SKIP SUBSEQUENT MESSAGES
EXFGLOB  EQU   EXFMALL+EXFSUPP+EXFRET  GLOBAL FLAGS
         POP   USING
         SPACE 2
***********************************************************************
*                                                                     *
*    CALL @@DYNAL,(ddn-len,ddn-adr,dsn-len,dsn-adr),VL                *
*                                                                     *
*    "-len" fields are self-defining values in the calling list,      *
*        or else pointers to 32-bit signed integer values             *
*                                                                     *
*    "ddn-adr"  is the address of the DD name to be used. When the    *
*        contents is hex zero or blank, and len=8, gets assigned.     *
*                                                                     *
*    "dsn-adr" is the address of a 1 to 44 byte data set name of an   *
*        existing file (sequential or partitioned).                   *
*                                                                     *
*    Calling @@DYNAL with a DDNAME and a zero length for the DSN      *
*    results in unallocation of that DD (and a PARM error).           *
*                                                                     *
*---------------------------------------------------------------------*
*                                                                     *
*     Author:  Gerhard Postpischil                                    *
*                                                                     *
*     This program is placed in the public domain.                    *
*                                                                     *
*---------------------------------------------------------------------*
*                                                                     *
*     Assembly: Any MVS or later assembler may be used.               *
*        Requires SYS1.MACLIB                                         *
*        Intended to work in any 24 and 31-bit environment.           *
*                                                                     *
*     Linker/Binder: RENT,REFR,REUS                                   *
*                                                                     *
*---------------------------------------------------------------------*
*     Return codes:  R15:04sssnnn   it's a program error code:        *
*     04804 - GETMAIN failed;  1400000n   PARM list error             *
*                                                                     *
*     Otherwise R15:0-1  the primary allocation return code, and      *
*       R15:2-3 the reason codes.                                     *
***********************************************************************
*  Maintenance:                                     new on 2008-06-07 *
*                                                                     *
***********************************************************************
@@DYNAL  FUNHEAD ,                DYNAMIC ALLOCATION
         LA    R11,16(,R13)       REMEMBER RETURN CODE ADDRESS
         MVC   0(4,R11),=X'04804000'  PRESET
         LR    R9,R1              SAVE PARAMETER LIST ADDRESS
         LA    R0,DYNALDLN        GET LENGTH OF SAVE AND WORK AREA
         GETMAIN RC,LV=(0)        GET STORAGE
         LTR   R15,R15            SUCCESSFUL ?
         BZ    DYNALHAV           YES
         STC   R15,3(,R11)        SET RETURN VALUES
         B     DYNALRET           RELOAD AND RETURN
*
*    CLEAR GOTTEN STORAGE AND ESTABLISH SAVE AREA
*
DYNALHAV ST    R1,8(,R13)         LINK OURS TO CALLER'S SAVE AREA
         ST    R13,4(,R1)         LINK CALLER'S TO OUR AREA
         LR    R13,R1
         USING DYNALWRK,R13
         MVC   0(4,R11),=X'14000001'  PRESET FOR PARM LIST ERROR
         MVC   DYNLIST(ALLDYNLN),PATLIST  INITIALIZE EVERYTHING
         LDADD R4,0(,R9)          DD NAME LENGTH
         L     R5,4(,R9)          -> DD NAME
         LDADD R6,8(,R9)          DSN LENGTH
         L     R7,12(,R9)         -> DATA SET NAME
*   NOTE THAT THE CALLER IS EITHER COMPILER CODE, OR A COMPILER
*   LIBRARY ROUTINE, SO WE DO MINIMAL VALIDITY CHECKING
*
*   PREPARE DYNAMIC ALLOCATION REQUEST LISTS
*
         LA    R0,ALLARB
         STCM  R0,7,ALLARBP+1     REQUEST POINTER
         LA    R0,ALLATXTP
         ST    R0,ALLARB+8        TEXT UNIT POINTER
         LA    R0,ALLAXDSN
         LA    R1,ALLAXDSP
         LA    R2,ALLAXDDN
         O     R2,=X'80000000'
         STM   R0,R2,ALLATXTP     TEXT UNIT ADDRESSES
*   COMPLETE REQUEST WITH CALLER'S DATA
*
         LTR   R4,R4              CHECK DDN LENGTH
         BNP   DYNALEXT           OOPS
         CH    R4,=AL2(L'ALLADDN)   REASONABLE SIZE ?
         BH    DYNALEXT           NO
         BCTR  R4,0
         EX    R4,DYNAXDDN        MOVE DD NAME
         OC    ALLADDN,=CL11' '   CONVERT HEX ZEROES TO BLANKS
         CLC   ALLADDN,=CL11' '   NAME SUPPLIED ?
         BNE   DYNALDDN           YES
         MVI   ALLAXDDN+1,DALRTDDN  REQUEST RETURN OF DD NAME
         CH    R4,=AL2(L'ALLADDN-1)   CORRECT SIZE FOR RETURN ?
         BE    DYNALNDD           AND LEAVE R5 NON-ZERO
         B     DYNALEXT           NO
DYNALDDN SR    R5,R5              SIGNAL NO FEEDBACK
*  WHEN USER SUPPLIES A DD NAME, DO AN UNCONDITIONAL UNALLOCATE ON IT
         LA    R0,ALLURB
         STCM  R0,7,ALLURBP+1     REQUEST POINTER
         LA    R0,ALLUTXTP
         ST    R0,ALLURB+8        TEXT UNIT POINTER
         LA    R2,ALLUXDDN
         O     R2,=X'80000000'
         ST    R2,ALLUTXTP        TEXT UNIT ADDRESS
         MVC   ALLUDDN,ALLADDN    SET DD NAME
         LA    R1,ALLURBP         POINT TO REQUEST BLOCK POINTER
         DYNALLOC ,               REQUEST ALLOCATION
DYNALNDD LTR   R6,R6              CHECK DSN LENGTH
         BNP   DYNALEXT           OOPS
         CH    R6,=AL2(L'ALLADSN)   REASONABLE SIZE ?
         BH    DYNALEXT           NO
         STH   R6,ALLADSN-2       SET LENGTH INTO TEXT UNIT
         BCTR  R6,0
         EX    R6,DYNAXDSN        MOVE DS NAME
*    ALLOCATE
         LA    R1,ALLARBP         POINT TO REQUEST BLOCK POINTER
         DYNALLOC ,               REQUEST ALLOCATION
         STH   R15,0(,R11)        PRIMARY RETURN CODE
         STH   R0,2(,R11)         REASON CODES
         LTR   R5,R5              NEED TO RETURN DDN ?
         BZ    DYNALEXT           NO
         MVC   0(8,R5),ALLADDN    RETURN NEW DDN, IF ANY
         B     DYNALEXT           AND RETURN
DYNAXDDN MVC   ALLADDN(0),0(R5)   COPY DD NAME
DYNAXDSN MVC   ALLADSN(0),0(R7)   COPY DATA SET NAME
*    PROGRAM EXIT, WITH APPROPRIATE RETURN CODES
*
DYNALEXT LR    R1,R13        COPY STORAGE ADDRESS
         L     R9,4(,R13)    GET CALLER'S SAVE AREA
         LA    R0,DYNALDLN   GET ORIGINAL LENGTH
         FREEMAIN R,A=(1),LV=(0)  AND RELEASE THE STORAGE
         LR    R13,R9        RESTORE CALLER'S SAVE AREA
DYNALRET FUNEXIT ,           RESTORE REGS; SET RETURN CODES
         LTORG ,
         PUSH  PRINT
         PRINT NOGEN         DON'T NEED TWO COPIES
PATLIST  DYNPAT P=PAT        EXPAND ALLOCATION DATA
         POP   PRINT
*    DYNAMICALLY ACQUIRED STORAGE
*
DYNALWRK DSECT ,             MAP STORAGE
         DS    18A           OUR OS SAVE AREA
DYNLIST  DYNPAT P=ALL        EXPAND ALLOCATION DATA
DYNALDLN EQU   *-DYNALWRK     LENGTH OF DYNAMIC STORAGE
MVSSUPA  CSECT ,             RESTORE
         SPACE 2
*
***********************************************************************
*                                                                     *
*  CALL @@SVC99,(rb)                                                  *
*                                                                     *
*  Execute DYNALLOC (SVC 99)                                          *
*                                                                     *
*  Caller must provide a request block, in conformance with the       *
*  MVS documentation for this (which is very complicated)             *
*                                                                     *
***********************************************************************
         PUSH  USING
         DROP  ,
         ENTRY @@SVC99
@@SVC99  DS    0H
         SAVE  (14,12),,@@SVC99   Save caller's regs.
         LR    R12,R15
         USING @@SVC99,R12
         LR    R11,R1
*
         GETMAIN RU,LV=WORKLEN,SP=SUBPOOL
         ST    R13,4(,R1)
         ST    R1,8(,R13)
         LR    R13,R1
         LR    R1,R11
         USING WORKAREA,R13
*
* Note that the SVC requires a pointer to the pointer to the RB.
* Because this function (not SVC) expects to receive a standard
* parameter list, where R1 so happens to be a pointer to the
* first parameter, which happens to be the address of the RB,
* then we already have in R1 exactly what SVC 99 needs.
*
* Except for one thing. Technically, you're meant to have the
* high bit of the pointer on. So we rely on the caller to have
* the parameter in writable storage so that we can ensure that
* we set that bit.
*
         L     R2,0(R1)
         O     R2,=X'80000000'
         ST    R2,0(R1)
         SVC   99
         LR    R2,R15
*
RETURN99 DS    0H
         LR    R1,R13
         L     R13,SAVEAREA+4
         FREEMAIN RU,LV=WORKLEN,A=(1),SP=SUBPOOL
*
         LR    R15,R2             Return success
         RETURN (14,12),RC=(15)   Return to caller
*
         DROP  R12
         POP   USING
         SPACE 2
***********************************************************************
*                                                                     *
*    CALL @@SNAP,snaplist                                             *
*                                                                     *
*    snaplist is the expansion produced by SNAP options,MF=L          *
*    Examples of use are in AOPEN and AREAD.                          *
*                                                                     *
*    Dump data are written to the SYSTERM DD, with predetermined      *
*        DCB values (required by SVC 51).                             *
*                                                                     *
*    according to my macro manual, SNAP will operate correctly with   *
*        addresses above the line, and only the DCB must be in 24-bit *
*        storage. If the MVS version doesn't work that way, code must *
*        be added to copy the caller's parm list to the DCB work area *
*                                                                     *
*    No output is produced unless the debug flag is on.               *
*                                                                     *
*    CODE IS NON-REENTRANT, NON-REFRESHABLE, but REUSABLE.            *
*                                                                     *
*---------------------------------------------------------------------*
*                                                                     *
*     Author:  Gerhard Postpischil                                    *
*                                                                     *
*     This code is placed in the public domain.                       *
*                                                                     *
*---------------------------------------------------------------------*
*                                                                     *
*     Return codes:  as set by SNAP macro/SVC                         *
*                                                                     *
***********************************************************************
*  Maintenance:                                     new on 2014-08-31 *
*                                                                     *
***********************************************************************
         SPACE 1
         PUSH  USING                                            GP14244
         PUSH  USING                                            GP14244
         PUSH  PRINT                                            GP14244
         PRINT NOGEN         DON'T NEED TWO COPIES              GP14244
         DROP  ,                                                GP14244
@@SNAP   FUNHEAD SAVE=(SNAPAREA,SNAPALEN,SUBPOOL)               GP14244
         L     R15,4(,R13)        GET CALLER'S SAVE AREA
         LA    R11,16(,R15)       REMEMBER RETURN CODE ADDRESS  GP14244
         SLR   R0,R0                                            GP14244
         ST    R0,0(,R11)         PRESET                        GP14244
         LA    R9,0(,R1)          SAVE PARAMETER LIST ADDRESS   GP14244
         LTR   R9,R9         REQUEST TO CLOSE/FREE?             GP14244
         BZ    SNAPCLOS        YES                              GP14244
         SPACE 1                                                GP14244
         L     R6,=A(@@BUGF)      GET DEBUGGING FLAG            GP14251
         TM    3(R6),X'01'        SNAP REQUESTED?               GP14244
         BZ    SNAPRET              NO; RETURN                  GP14244
         ICM   R10,15,@SNAPDCB    PREVIOUSLY GOTTEN?            GP14244
         BNZ   SNAPGOT                                          GP14244
         USING SNAPDCB,R10   DECLARE DYNAMIC WORK AREA          GP14244
SNAPGET  LA    R0,SNAPSLEN   GET LENGTH OF SAVE AND WORK AREA   GP14244
         AIF   ('&ZSYS' EQ 'S370').SNPDEF                       GP14244
         GETMAIN RU,LV=(0),SP=SUBPOOL,LOC=BELOW                 GP14244
         AGO   .SNPCOM                                          GP14244
.SNPDEF  GETMAIN RU,LV=(0),SP=SUBPOOL                           GP14244
.SNPCOM  STM   R0,R1,#SNAPDCB     SAVE FOR RELEASE              GP14244
         LR    R10,R1                                           GP14244
         MVC   SNAPDCB(PATSNAPL),PATSNAP   INIT DCB, ETC.       GP14244
         OPEN  (SNAPDCB,OUTPUT),MF=(E,SNAPOCL)                  GP14244
         SPACE 1                                                GP14244
         LTR   R9,R9         ANY ADDRESS ?                      GP14244
         BZ    SNAPCLOS        NO; CLOSE REQUEST                GP14244
SNAPGOT  LA    R7,1          INCREMENT DUMP COUNTER             GP14244
         AL    R7,SNAPCTR    INCREMENT DUMP COUNTER             GP14244
         ST    R7,SNAPCTR    INCREMENT DUMP COUNTER             GP14244
         SNAP  DCB=SNAPDCB,ID=(R7),MF=(E,(R9))                  GP14244
         ST    R15,0(,R11)   PROPAGATE RETUNR CODE              GP14244
         B     SNAPRET                                          GP14244
         SPACE 1                                                GP14244
SNAPCLOS ICM   R10,15,@SNAPDCB    EVER GOTTEN STORAGE ?         GP14244
         BZ    SNAPRET              NO; JUST RETURN             GP14244
         TM    SNAPDCB+DCBOFLGS-IHADCB,DCBOFOPN  OPEN ?         GP14244
         BZ    SNAPFREE             NO; JUST FREE STORAGE       GP14244
         CLOSE MF=(E,SNAPOCL)                                   GP14244
SNAPFREE L     R0,#SNAPDCB                                      GP14244
         FREEMAIN R,LV=(0),A=(R10)                              GP14244
         XC    #SNAPDCB(L'#SNAPDCB+L'@SNAPDCB),#SNAPDCB         GP14244
         SPACE 1                                                GP14244
SNAPRET  FUNEXIT ,           RESTORE REGS; SET RETURN CODES     GP14244
         SPACE 1                                                GP14244
         LTORG ,                                                GP14244
         SPACE 1                                                GP14244
#SNAPDCB DC    F'0'    1/2   LENGTH OF PERSISTENT DCB WORK AREA GP14244
@SNAPDCB DC    A(0)    2/2   ADDR. OF PERSISTENT DCB WORK AREA  GP14244
         SPACE 1                                                GP14244
PATSNAP  DCB   DDNAME=SYSTERM,MACRF=(W),DSORG=PS,                      *
               RECFM=VBA,LRECL=125,BLKSIZE=1632  882            GP14244
PATSNOC  OPEN  (PATSNAP,OUTPUT),MF=L                            GP14244
PATSCTR  DC    F'0'          DUMP ID; WRAPS AT 256->0           GP14244
PATSNAPL EQU   *-PATSNAP                                        GP14244
         SPACE 1                                                GP14244
         SPACE 1                                                GP14244
SNAPSAVE DSECT ,                                                GP14244
SNAPDCB  DCB   DDNAME=SYSTERM,MACRF=(W),DSORG=PS,                      *
               RECFM=VBA,LRECL=125,BLKSIZE=1632  882            GP14244
SNAPOCL  OPEN  (SNAPDCB,OUTPUT),MF=L                            GP14244
SNAPCTR  DC    F'0'          DUMP ID; WRAPS AT 256->0           GP14244
SNAPSLEN EQU   *-SNAPSAVE                                       GP14244
         SPACE 1                                                GP14244
SNAPAREA DSECT ,                                                GP14244
         DS    18A           STANDARD SAVE AREA ONLY            GP14244
SNAPALEN EQU   *-SNAPAREA    LENGTH TO GET                      GP14244
         POP   USING                                            GP14244
         POP   PRINT                                            GP14244
MVSSUPA  CSECT ,             RESTORE CSECT                      GP14244
         SPACE 2
*
* Keep this code last because it makes no difference - no USINGs
*
***********************************************************************
*                                                                     *
*  SETJ - SAVE REGISTERS INTO ENV                                     *
*                                                                     *
***********************************************************************
         ENTRY @@SETJ
@@SETJ   L     R15,0(,R1)         get the env variable
         STM   R0,R14,0(R15)      save registers to be restored
         LA    R15,0              setjmp needs to return 0
         BR    R14                return to caller
         SPACE 1
***********************************************************************
*                                                                     *
*  LONGJ - RESTORE REGISTERS FROM ENV                                 *
*                                                                     *
***********************************************************************
         ENTRY @@LONGJ
@@LONGJ  L     R2,0(,R1)          get the env variable
         L     R15,60(,R2)        get the return code
         LM    R0,R14,0(R2)       restore registers
         BR    R14                return to caller
         SPACE 2
*
* S/370 doesn't support switching modes so this code is useless,
* and won't compile anyway because "BSM" is not known.
*
         AIF   ('&ZSYS' EQ 'S370').NOMODE If S/370 we can't switch mode
         PUSH  USING
         DROP  ,
***********************************************************************
*                                                                     *
*  SETM24 - Set AMODE to 24                                           *
*                                                                     *
***********************************************************************
         ENTRY @@SETM24
         USING @@SETM24,R15
@@SETM24 LA    R14,0(,R14)        Sure hope caller is below the line
         BSM   0,R14              Return in amode 24
         POP   USING
         SPACE 1
         PUSH  USING
         DROP  ,
***********************************************************************
*                                                                     *
*  SETM31 - Set AMODE to 31                                           *
*                                                                     *
***********************************************************************
         ENTRY @@SETM31
         USING @@SETM31,R15
@@SETM31 ICM   R14,8,=X'80'       Clobber entire high byte of R14
*                                 This is necessary because if people
*                                 use BALR in 24-bit mode, the address
*                                 will have rubbish in the high byte.
*                                 People switching between 24-bit and
*                                 31-bit will be RMODE 24 anyway, so
*                                 there is nothing to preserve in the
*                                 high byte.
         BSM   0,R14              Return in amode 31
         LTORG ,
         POP   USING
*
.NOMODE  ANOP  ,                  S/370 doesn't support MODE switching
*
*
*
***********************************************************************
***********************************************************************
*                                                                     *
* End of functions, start of data areas                               *
*                                                                     *
***********************************************************************
***********************************************************************
         SPACE 2
*
***********************************************************************
*                                                                     *
*  The work area includes both a register save area and various       *
*  variables used by the different routines.                          *
*                                                                     *
***********************************************************************
WORKAREA DSECT
SAVEAREA DS    18F
DWORK    DS    D                  Extra work space
WWORK    DS    D                  Extra work space
DWDDNAM  DS    D                  Extra work space
WORKLEN  EQU   *-WORKAREA
SAVOSUB  DS    6A         R10-R15 Return saver for AOPEN subs   GP14234
MYJFCB   DS    0F
         IEFJFCBN LIST=YES        Job File Control Block
CAMLST   DS    XL(CAMLEN)         CAMLST for OBTAIN to get VTOC entry
* Format 1 Data Set Control Block
*   N.B. Current program logic does not use DS1DSNAM, leaving 44 bytes
*     of available space
         IECSDSL1 1               Map the Format 1 DSCB
DSCBCCHH DS    CL5                CCHHR of DSCB returned by OBTAIN
         DS    CL47               Rest of OBTAIN's 148 byte work area
         ORG   DS1FMTID                                         GP14213
         IECSDSL1 4               Redefine for VTOC             GP14213
         ORG   ,                                                GP14213
         SPACE 1
*   DEFINITIONS TO ALLOW ASSEMBLY AND TESTING OF SMS, ETC. UNDER
*   MVS 3.n
*
FM1FLAG  EQU   DS1NOBDB+1,1,C'X'  MORE FLAGS                    GP14205
FM1COMPR EQU   X'80'           COMPRESSABLE EXTENDED IF DS1STRP GP14205
FM1CPOIT EQU   X'40'           CHECKPOINTED D S                 GP14205
FM1SMSFG EQU   FM1FLAG+1,1,C'X'  SMS FLAG                       GP14205
FM1SMSDS EQU   X'80'           SMS D S                          GP14205
FM1SMSUC EQU   X'40'           NO BCS ENTRY                     GP14205
FM1REBLK EQU   X'20'           MAY BE REBLOCKED                 GP14205
FM1CRSDB EQU   X'10'           BLKSZ BY DADSM                   GP14205
FM1PDSE  EQU   X'08'           PDS/E                            GP14205
FM1STRP  EQU   X'04'           EXTENDED FORMAT D S              GP14205
FM1PDSEX EQU   X'02'           HFS D S                          GP14205
FM1DSAE  EQU   X'01'           EXTENDED ATTRIBUTES EXISY        GP14205
FM1SCEXT EQU   FM1SMSFG+1,3,C'X'  SECONDARY SPACE EXTENSION     GP14205
FM1SCXTF EQU   FM1SCEXT,1,C'X'  -"- FLAG                        GP14205
FM1SCAVB EQU   X'80'           SCXTV IS AVG BLOCK LEN           GP14205
FM1SCMB  EQU   X'40'                 IS IN MEGBYTES             GP14205
FM1SCKB  EQU   X'20'                 IS IN KILOBYTES            GP14205
FM1SCUB  EQU   X'10'                 IS IN BYTES                GP14205
FM1SCCP1 EQU   X'08'           SCXTV COMPACTED BY 256           GP14205
FM1SCCP2 EQU   X'04'                 COMPACTED BY 65536         GP14205
FM1SCXTV EQU   FM1SCXTF+1,2,C'X'  SEC SPACE EXTNSION VALUE      GP14205
FM1ORGAM EQU   DS1ACBM         CONSISTENT NAMING - VSAM D S     GP14205
FM1RECFF EQU   X'80'           RECFM F                          GP14205
FM1RECFV EQU   X'40'           RECFM V                          GP14205
FM1RECFU EQU   X'C0'           RECFM U                          GP14205
FM1RECFT EQU   X'20'           RECFM T   001X XXXX IS D         GP14205
FM1RECFB EQU   X'10'           RECFM B                          GP14205
FM1RECFS EQU   X'08'           RECFM S                          GP14205
FM1RECFA EQU   X'04'           RECFM A                          GP14205
FM1RECMC EQU   X'02'           RECFM M                          GP14205
*   OPTCD DEFINITIONS   BDAM    W.EFA..R                        GP14205
*                       ISAM    WUMIY.LR                        GP14205
*             BPAM/BSAM/QSAM    WUCHBZTJ                        GP14205
FM1OPTIC EQU   X'80'  FOR DS1ORGAM - CATLG IN ICF CAT           GP14205
FM1OPTBC EQU   X'40'           ICF CATALOG                      GP14205
FM1RACDF EQU   DS1IND40                                         GP14205
FM1SECTY EQU   DS1IND10                                         GP14205
FM1WRSEC EQU   DS1IND04                                         GP14205
FM1SCAL1 EQU   DS1SCALO,1,C'X'    SEC. ALLOC FLAGS              GP14205
FM1DSPAC EQU   X'C0'         SPACE REQUEST MASK                 GP14205
FM1CYL   EQU   X'C0'           CYLINDER BOUND                   GP14205
FM1TRK   EQU   X'80'           TRACK                            GP14205
FM1AVRND EQU   X'41'           AVG BLOCK + ROUND                GP14205
FM1AVR   EQU   X'40'           AVG BLOCK LEN                    GP14205
FM1MSGP  EQU   X'20'                                            GP14205
FM1EXT   EQU   X'10'           SEC. EXTENSION EXISTS            GP14205
FM1CONTG EQU   X'08'           REQ. CONTIGUOUS                  GP14205
FM1MXIG  EQU   X'04'           MAX                              GP14205
FM1ALX   EQU   X'02'           ALX                              GP14205
FM1DSABS EQU   X'00'           ABSOLUTE TRACK                   GP14205
FM1SCAL3 EQU   FM1SCAL1+1,3,C'X'  SEC ALLOC QUANTITY            GP14205
         SPACE 1
DDWATTR  DS    16XL8         DS ATTRIBUTES (DSORG,RECFM,X,LRECL,BLKSI)
BLDLLIST DS    Y(1,12+2+31*2)     BLDL LIST HEADER              GP14205
BLDLNAME DS    CL8' ',XL(4+2+31*2)    MEMBER NAME AND DATA      GP14205
         AGO   .COMSWA  replaced SWA for cross-assembly compatibility
*COMP*   AIF   ('&ZSYS' NE 'S390').COMSWA                       GP14205
DDWEPA   DS    A(DDWSVA)                                        GP14205
DDWSWA   SWAREQ FCODE=RL,EPA=DDWEPA,MF=L                        GP14205
DDWSVA   DS    7A                 (IBM LIES ABOUT 4A)           GP14205
DDWSWAL  EQU   *-DDWSWA           LENGTH TO CLEAR               GP14205
.COMSWA  SPACE 1                                                GP14205
ZEROES   DS    F             CONSTANT
DDWBLOCK DS    F             MAXIMUM BUFFER SIZE NEEDED         GP14205
DDWFLAGS DS    X             RESULT FLAGS FOR ALL               GP14205
DDWFLAG1 DS    X             RESULT FLAGS FOR FIRST DD          GP14205
DDWFLAG2 DS    X             RESULT FLAGS FOR ALL BUT FIRST     GP14205
CWFDD    EQU   X'80'           FOUND A DD                       GP14205
CWFCONC  EQU   CWFDD           AFTER FLAG MERGE - CONCATENATED  GP14205
CWFSEQ   EQU   X'40'           USE IS SEQUENTIAL                GP14205
CWFPDQ   EQU   X'20'           DS IS PDS WITH MEMBER NAME       GP14205
CWFPDS   EQU   X'10'           DS IS PDS (or PDS/E with S390)   GP14205
CWFVSM   EQU   X'08'           DS IS VSAM (limited support)     GP14205
CWFVTOC  EQU   X'04'           DS IS VTOC (limited support)     GP14205
CWFBLK   EQU   X'02'           DD HAS FORCED BLKSIZE            GP14205
OPERF    DS    X             ERROR CONDITIONS                   GP14205
ORFBADNM EQU   04            DD name <= blank                   GP14205
ORFNODD  EQU   08            DD name not found in TIOT          GP14205
ORFNOJFC EQU   12            Error getting JFCB                 GP14205
ORFNODS1 EQU   16            Error getting DSCB 1               GP14205
ORFBATIO EQU   20            Unusable TIOT entry                GP14205
ORFBADSO EQU   24            Invalid or unsupported DSORG       GP14205
ORFBADCB EQU   28            Invalid DCB parameters             GP14205
ORFBATY3 EQU   32            Unsupported unit type (Graf, Comm, etc.)
ORFBACON EQU   36            Invalid concatenation              GP14205
ORFBDMOD EQU   40            Invalid MODE for DD/function       GP14205
ORFBDPDS EQU   44            PDS not initialized                GP14205
ORFBDDIR EQU   48            PDS not initialized                GP14205
ORFNOSTO EQU   52            Out of memory                      GP14205
ORFNOMEM EQU   68            Member not found (BLDL/FIND)       GP14205
ORFBDMEM EQU   72            Member not permitted (seq.)        GP14205
         SPACE 1
TRUENAME DS    CL44               DS name for alias on DD       GP14233
CATWORK  DS    ((265+7)/8)D'0'    LOCATE work area              GP14233
         ORG   CATWORK            Redefine returned data        GP14233
CAWCOUNT DS    H                  Number of entries returned    GP14233
CAW#VOL  DS    H                  Number of volumes in this DS  GP14233
CAWTYPE  DS    XL4                Unit type                     GP14233
CAWSER   DS    CL6                Volume serial                 GP14233
CAWFILE  DS    XL2                Tape file number              GP14233
         ORG   ,                                                GP14233
OPENLEN  EQU   *-WORKAREA         Length for @@AOPEN processing
         SPACE 2
***********************************************************************
*                                                                     *
* ZDCBAREA - the address of this memory is used by the C caller       *
* as a "handle". The block of memory has different contents depending *
* on what sort of file is being opened, but it will be whatever the   *
* assembler code is expecting, and the caller merely needs to         *
* provide the handle (returned from AOPEN) in subsequent calls so     *
* that the assembler can keep track of things.                        *
*                                                                     *
*   FILE ACCESS CONTROL BLOCK (N.B.-STARTS WITH DCBD DUE TO DSECT)    *
*   CONTAINS DCB, DECB, JFCB, DSCB 1, BUFFER POINTERS, FLAGS, ETC.    *
*                                                                     *
***********************************************************************
         DCBD  DSORG=PS,DEVD=(DA,TA)   Map Data Control Block
         ORG   IHADCB             Overlay the DCB DSECT
ZDCBAREA DS    0H
         DS    CL(BPAMDCBL)       Room for BPAM DCB             GP14205
         READ  DECB,SF,IHADCB,2-2,3-3,MF=L  READ/WRITE BSAM     GP14205
         ORG   IHADCB             Only using one DCB
         DS    CL(QSAMDCBL)         so overlay this one
         ORG   IHADCB             Only using one DCB            GP14205
         DS    CL(BSAMDCBL)
         ORG   IHADCB             Only using one DCB            GP14233
ZAACB    DS    CL(VSAMDCBL)       VSAM ACB                      GP14233
ZARPL    RPL   ACB=ZAACB,OPTCD=(SEQ,SYN,LOC)                    GP14244
ZAMODCB  DS    XL(ZAMODCBL)  MODCB WORK AREA                    GP14233
ZASHOCB  DS    XL(ZASHOCBL)  SHOCB WORK AREA                    GP14233
ZAARG    DS    A                  Pointer                       GP14233
ZARRN    DS    F                  Relative record number        GP14233
         SPACE 2
         ORG   IHADCB             Only using one DCB
TAPEDCB  DCB   DDNAME=TAPE,MACRF=E,DSORG=PS,REPOS=Y,BLKSIZE=0,         *
               DEVD=TA                 LARGE SIZE
         ORG   TAPEDCB+84    LEAVE ROOM FOR DCBLRECL
ZXCPVOLS DC    F'0'          VOLUME COUNT
TAPECCW  CCW   1,3-3,X'40',4-4
         CCW   3,3-3,X'20',1
TAPEXLEN EQU   *-TAPEDCB     PATTERN TO MOVE
TAPEECB  DC    A(0)
TAPEIOB  DC    X'42,00,00,00'
         DC    A(TAPEECB)
         DC    2A(0)
         DC    A(TAPECCW)
         DC    A(TAPEDCB)
         DC    2A(0)
         SPACE 1
         ORG   IHADCB
ZPUTLINE PUTLINE MF=L        PATTERN FOR TERMINAL I/O
*DSECT*  IKJIOPL ,
         SPACE 1
ZIOPL    DS    0A            MANUAL EXPANSION TO AVOID DSECT
IOPLUPT  DS    A        PTR TO UPT
IOPLECT  DS    A        PTR TO ECT
IOPLECB  DS    A        PTR TO USER'S ECB
IOPLIOPB DS    A        PTR TO THE I/O SERVICE RTN PARM BLOCK
ZIOECB   DS    A                   TPUT ECB
ZIOECT   DS    A                   ORIGINATING ECT
ZIOUPT   DS    A                   UPT
ZIODDNM  DS    CL8      DD NAME AT OFFSET X'28' FOR DCB COMPAT.
ZGETLINE GETLINE MF=L             TWO WORD GTPB
         SPACE 2
*   VTOC READ ACCESS - INTERLEAVE WITH BSAM DCB
*
         ORG   IHADCB                                           GP14213
ZVCPVOL  DS    H                  Cylinder per volume           GP14213
ZVTPCYL  DS    H                  Tracks per cylinder           GP14213
ZVLOCCHH DS    XL4     1/3        CCHH of VTOC start            GP14213
ZVHICCHH DS    XL4     2/3        CCHH of VTOC end              GP14213
ZVHIREC  DS    X       3/3        High record on track          GP14213
         DS    0H                 Align for STH                 GP14213
ZVUSCCHH DS    XL5                Address of current record     GP14213
ZVSER    DS    CL6                Volume serial                 GP14213
ZVSEEK   CAMLST SEEK,1-1,2-2,3-3  CAMLST to SEEK by address     GP14213
         SPACE 2
         ORG   ,
OPENCLOS DS    A                  OPEN/CLOSE parameter list
DCBXLST  DS    2A                 07 JFCB / 85 DCB EXIT
EOFR24   DS    CL(EOFRLEN)
         AIF   ('&ZSYS' EQ 'S370').NOD24  If S/370, no 24-bit OPEN exit
         DS    0H
DOPE24   DS    CL(DOPELEN)        DCB open 24-bit code
DOPE31   DS    A                  Address of DCB open exit
.NOD24   ANOP  ,
ZBUFF1   DS    A,F                Address, length of buffer
ZBUFF2   DS    A,F                Address, length of 2nd buffer
KEPTREC  DS    A,F                Address & length of saved rcd
*
         MAPSUPRM DSECT=NO,PFX=ZP      MAP MODE WORK AREA       GP14233
BUFFADDR DS    A     1/3          Location of the BLOCK Buffer
BUFFCURR DS    A     2/3          Current record in the buffer
BUFFEND  DS    A     3/3          Address after end of current block
VBSADDR  DS    A                  Location of the VBS record build area
VBSEND   DS    A                  Addr. after end VBS record build area
         SPACE 1
ZWORK    DS    D             Below the line work storage
ZDDN     DS    CL8           DD NAME                            GP14205
ZMEM     DS    CL8           MEMBER NAME or nulls               GP14205
DEVINFO  DS    2F                 UCB Type / Max block size
         SPACE 1
RECFMIX  DS    X             Record format index: 0-F 4-V 8-U
IXFIX    EQU   0               Recfm = F                        GP14213
IXVAR    EQU   4               Recfm = V                        GP14213
IXUND    EQU   8               Recfm = U                        GP14213
IOIX     DS    X               I/O routine index                GP14213
IXSAM    EQU   0               BSAM/BPAM - default              GP14213
IXVSM    EQU   4               VSAM data set                    GP14213
IXVTC    EQU   8               VTOC reader                      GP14213
IXTRM    EQU   12              TSO terminal                     GP14213
         SPACE 1
ZDVTYPE  DS    X      1/4    Device type of first/only DD       GP14251
         SPACE 1
ZRECFM   DS    X      2/4    Equivalent RECFM bits
         SPACE 1
IOSFLAGS DS    X      3/4    Additional MODE related flags      GP14251
IOFVSAM  EQU   X'04'           Use VSAM                         GP14233
         SPACE 1
IOMFLAGS DS    X      4/4    Remember open MODE
IOFTERM  EQU   X'80'           Using GETLINE/PUTLINE
IOFBPAM  EQU   X'20'           Unlike BPAM concat - special handling
IOFBLOCK EQU   X'10'           Using BSAM READ/WRITE mode
IOFEXCP  EQU   X'08'           Use EXCP for TAPE
IOFOUT   EQU   X'01'           Output mode
         SPACE 1
IOPFLAGS DS    X             Remember prior events
IOFLEOF  EQU   X'80'           Encountered an End-of-File
IOFLSDW  EQU   X'40'           Spanned record incomplete
IOFLDATA EQU   X'20'           Output buffer has data
IOFLIOWR EQU   X'10'           Last I/O was Write type
IOFCURSE EQU   X'08'           Write buffer recursion
IOFDCBEX EQU   X'04'           DCB exit entered
IOFCONCT EQU   X'02'           Reread - unlike concatenation
IOFKEPT  EQU   X'01'           Record info kept
         SPACE 1
FILEMODE DS    X             AOPEN requested record format default
FMFIX    EQU   0               Fixed RECFM (blocked)
FMVAR    EQU   1               Variable (blocked)
FMUND    EQU   2               Undefined
ZDDFLAGS DS    X             RESULT FLAGS FOR ALL               GP14244
ZIOSAVE2 DS    18F           Save area for physical write
SAVEADCB DS    18F                Register save area for PUT
ZDCBLEN  EQU   *-ZDCBAREA
*
* End of handle/DCB area
*
*
*
         SPACE 2
         PRINT NOGEN
         IHAPSA ,            MAP LOW STORAGE
         CVT DSECT=YES
         IKJTCB ,            MAP TASK CONTROL BLOCK
         IKJECT ,            MAP ENV. CONTROL BLOCK
         IKJPTPB ,           PUTLINE PARAMETER BLOCK
         IKJCPPL ,
         IKJPSCB ,
         IEZJSCB ,
         IEZIOB ,
         IEFZB4D0 ,          MAP SVC 99 PARAMETER LIST
         IEFZB4D2 ,          MAP SVC 99 PARAMETERS
MYUCB    DSECT ,
         IEFUCBOB ,
MYTIOT   DSECT ,
         IEFTIOT1 ,
         IHAPDS PDSBLDL=YES
         SPACE 1
         IFGACB ,                                               GP14233
         SPACE 1
         IFGRPL ,                                               GP14233
         AIF   ('&ZSYS' NE 'S390').MVSEND
         IEFJESCT ,
.MVSEND  END   ,
