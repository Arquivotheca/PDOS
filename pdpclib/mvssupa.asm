MVSSUPA  TITLE 'M V S S U P A  ***  MVS VERSION OF PDP CLIB SUPPORT'
         MACRO ,                                                GP09286
&NM      FIXWRITE ,                                             GP09286
&NM      L     R15,=A(TRUNCOUT)                                 GP09286
         BALR  R14,R15       TRUNCATE CURRENT WRITE BLOCK       GP09286
         MEND  ,                                                GP09286
         MACRO ,
&NM      SAM31 &WORK=R15                             ADDED ON GP2009225
         GBLC  &SYS
.*
.*   SAM31 sets addressing mode to 31 for S380, etc.
.*         expands nothing or label for S370
.*         is never invoked by HLASM (valid hardware instruction)
.*
         AIF   ('&SYS' EQ 'S370').OLDSYS
&NM      LA    &WORK,*+10    GET PAST BSM WITH BIT 0 ON
         O     &WORK,=X'80000000'  SET MODE BIT
         DC    0H'0',AL.4(0,X'0B',0,&WORK)  CONTINUE IN 31-BIT MODE
         MEXIT ,
.OLDSYS  AIF   ('&NM' EQ '').MEND
&NM      DS    0H            DEFINE LABEL ONLY
.MEND    MEND  ,
         MACRO ,
&NM      SAM24 &WORK=R15                             ADDED ON GP2009225
         GBLC  &SYS
.*
.*   SAM24 sets addressing mode to 24 for S380, etc.
.*         expands nothing or label for S370
.*         is never invoked by HLASM (valid hardware instruction)
.*
         AIF   ('&SYS' EQ 'S370').OLDSYS
&NM      LA    &WORK,*+6     GET PAST BSM WITH BIT 0 OFF
         DC    0H'0',AL.4(0,X'0B',0,&WORK)  CONTINUE IN 24-BIT MODE
         MEXIT ,
.OLDSYS  AIF   ('&NM' EQ '').MEND
&NM      DS    0H            DEFINE LABEL ONLY
.MEND    MEND  ,
***********************************************************************
*
*  This program written by Paul Edwards.
*  Released to the public domain
*
*  Extensively modified by others
*
***********************************************************************
*
*  MVSSUPA - Support routines for PDPCLIB under MVS
*
*  It is currently coded for GCC, but C/370 functionality is
*  still there, it's just not being tested after any change.
*
***********************************************************************
*
* Note that the VBS support may not be properly implemented.
* Note that this code issues WTOs. It should be changed to just
* set a return code and exit gracefully instead.
*
* Here are some of the errors reported:
*
*  OPEN input failed return code is: -37
*  OPEN output failed return code is: -39
*
* FIND input member return codes are:
* Original, before the return and reason codes had
* negative translations added refer to copyrighted:
* DFSMS Macro Instructions for Data Sets
* RC = 0 Member was found.
* RC = -1024 Member not found.
* RC = -1028 RACF allows PDSE EXECUTE, not PDSE READ.
* RC = -1032 PDSE share not available.
* RC = -1036 PDSE is OPENed output to a different member.
* RC = -2048 Directory I/O error.
* RC = -2052 Out of virtual storage.
* RC = -2056 Invalid DEB or DEB not on TCB or TCBs DEB chain.
* RC = -2060 PDSE I/O error flushing system buffers.
* RC = -2064 Invalid FIND, no DCB address.
*
***********************************************************************
*   Changes by Gerhard Postpischil:
*     EQU * for entry points deleted (placed labels on SAVE) to avoid
*       0C6 abends when EQU follows a LTORG
*     Fixed 0C4 abend in RECFM=Vxxx processing; fixed PUT length error.
*     Deleted unnecessary and duplicated instructions
*     Added @@SYSTEM and @@DYNAL routines                2008-06-10
*     Added @@IDCAMS non-reentrant, non-refreshable      2008-06-17
*     Modified I/O for BSAM, EXCP, and terminal I/O             GP09286
***********************************************************************
*
         MACRO ,             COMPILER DEPENDENT LOAD INTEGER
&NM      LDINT &R,&A         LOAD INTEGER VALUE FROM PARM LIST
         GBLC  &COMP         COMPILER GCC OR C/370
&NM      L     &R,&A         LOAD PARM VALUE
         AIF ('&COMP' EQ 'GCC').MEND
* THIS LINE IS FOR ANYTHING NOT GCC: C/370
         L     &R,0(,&R)     LOAD INTEGER VALUE
.MEND    MEND  ,
         MACRO ,             PATTERN FOR @@DYNAL'S DYNAMIC WORK AREA
&NM      DYNPAT &P=MISSING-PFX
.*   NOTE THAT EXTRA FIELDS ARE DEFINED FOR FUTURE EXPANSION
.*
&NM      DS    0D            ALLOCATION FIELDS
&P.ARBP  DC    0F'0',X'80',AL3(&P.ARB) RB POINTER
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
&P.URBP  DC    0F'0',X'80',AL3(&P.URB) RB POINTER
&P.URB   DC    0F'0',AL1(20,S99VRBUN,0,0)
         DC    A(0,&P.UTXTP,0,0)       SVC 99 REQUEST BLOCK
&P.UTXTP DC    X'80',AL3(&P.UXDDN)
&P.UXDDN DC    Y(DUNDDNAM,1,8)
&P.UDDN  DC    CL8' '        RETURNED DDNAME
&P.ULEN  EQU   *-&P.URBP       LENGTH OF REQUEST BLOCK
&P.DYNLN EQU   *-&P.ARBP     LENGTH OF ALL DATA
         MEND  ,
         SPACE 1
         COPY  MVSMACS
         COPY  PDPTOP
         SPACE 1
         GBLC  &LOCLOW,&LOCANY    MAKE GETMAINS EASIER          GP09286
         AIF   ('&SYS' EQ 'S370').NOLOCS                        GP09286
&LOCLOW  SETC  'LOC=BELOW'                                      GP09286
&LOCANY  SETC  'LOC=ANY'                                        GP09286
.NOLOCS  SPACE 1
MVSSUPA  CSECT ,                                                GP09286
         PRINT GEN
         YREGS IS NOW AVAILABLE IN MVS MACLIB/MODGENM           GP09286
         SPACE 1
*-----------------------ASSEMBLY OPTIONS------------------------------*
SUBPOOL  EQU   0                                                      *
*---------------------------------------------------------------------*
         SPACE 1
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  AOPEN- Open a data set
*
*  Parameters are:
*1 DDNAME - space-padded, 8 character DDNAME to be opened
*2 MODE =  0 INPUT  1 OUTPUT  2 UPDAT   3 APPEND      Record mode
*  MODE =  4 INOUT  5 OUTIN
*  MODE = 8/9 Use EXCP for tape, BSAM otherwise (or 32<=JFCPNCP<=65)
*  MODE + 10 = Use BLOCK mode (valid 10-15)
*  MODE = 80 = GETLINE, 81 = PUTLINE (other bits ignored)
*3 RECFM - 0 = F, 1 = V, 2 = U. Default/preference set by caller;
*                               actual value returned from open.
*4 LRECL   - Default/preference set by caller; OPEN value returned.
*5 BLKSIZE - Default/preference set by caller; OPEN value returned.
*
*  August 2009 revision - caller will pass preferred RECFM (coded 0-2)
*    LRECL, and BLKSIZE values. DCB OPEN exit OCDCBEX will use these
*    defaults when not specified on JCL or DSCB merge.          GP09286
*
*6 ZBUFF2 - pointer to an area that may be written to (size is LRECL)
*7 MEMBER - *pointer* to space-padded, 8 character member name.
*    If pointer is 0 (NULL), no member is requested
*
*  Return value:
*  An internal "handle" that allows the assembler routines to
*  keep track of what's what when READ etc are subsequently
*  called.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
         PUSH  USING                                            GP09286
@@AOPEN  FUNHEAD SAVE=(WORKAREA,OPENLEN,SUBPOOL)                GP09286
         LR    R11,R1             KEEP R11 FOR PARAMETERS
         USING PARMSECT,R11       MAKE IT EASIER TO READ        GP09286
         L     R3,PARM1           R3 POINTS TO DDNAME
* Note that R5 is used as a scratch register
         L     R8,PARM4           R8 POINTS TO LRECL
* PARM5    has BLKSIZE
* PARM6    has ZBUFF2 pointer
         L     R9,PARM7           R9 POINTS TO MEMBER NAME (OF PDS)
         LA    R9,00(,R9)         Strip off high-order bit or byte
         SPACE 1
         L     R4,PARM2           R4 is the MODE.  0=input 1=output
         CH    R4,=H'256'         Call with value?              GP09286
         BL    *+8                Yes; else pointer
         L     R4,0(,R4)          Load C/370 MODE.  0=input 1=output
         SPACE 1
         GETMAIN RU,LV=ZDCBLEN,SP=SUBPOOL,&LOCLOW               GP09286
         LR    R10,R1             Addr.of storage obtained to its base
         USING IHADCB,R10         Give assembler DCB area base register
         LR    R0,R10             Load output DCB area address
         LA    R1,ZDCBLEN         Load output length of DCB area
         LA    R15,0              Pad of X'00' and no input length
         MVCL  R0,R14             Clear DCB area to binary zeroes
*---------------------------------------------------------------------*
*   GET USER'S DEFAULTS HERE, BECAUSE THEY MAY GET CHANGED      GP09286
*---------------------------------------------------------------------*
         L     R5,PARM3    HAS RECFM code (0-FB 1-VB 2-U)
         L     R14,0(,R5)         LOAD RECFM VALUE              GP09286
         STC   R14,FILEMODE       PASS TO OPEN                  GP09286
         L     R14,0(,R8)         GET LRECL VALUE               GP09286
         ST    R14,LRECL          PASS TO OPEN                  GP09286
         L     R14,PARM5          R14 POINTS TO BLKSIZE         GP09286
         L     R14,0(,R14)        GET BLOCK SIZE                GP09286
         ST    R14,BLKSIZE        PASS TO OPEN                  GP09286
         SPACE 1
*---------------------------------------------------------------------*
*   DO THE DEVICE TYPE NOW TO CHECK WHETHER EXCP IS POSSIBLE    GP09286
*     ALSO BYPASS STUFF IF USER REQUESTED TERMINAL I/O          GP09286
*---------------------------------------------------------------------*
OPCURSE  STC   R4,WWORK           Save to storage               GP09286
         STC   R4,WWORK+1         Save to storage               GP09286
         NI    WWORK+1,7          Retain only open mode bits    GP09286
         TM    WWORK,IOFTERM      Terminal I/O ?                GP09286
         BNZ   TERMOPEN           Yes; do completely different  GP09286
         MVC   DWDDNAM,0(R3)      Move below the line           GP09296
         DEVTYPE DWDDNAM,DWORK    Check device type             GP09296
         BXH   R15,R15,FAILDCB    DD missing                    GP09286
         ICM   R0,15,DWORK+4      Any device size ?             GP09286
         BNZ   OPHVMAXS                                         GP09286
         MVC   DWORK+6(2),=H'32760'    Set default max          GP09286
         SPACE 1
OPHVMAXS CLI   WWORK+1,3          Append requested ?            GP09286
         BNE   OPNOTAP            No                            GP09286
         TM    DWORK+2,UCB3TAPE+UCB3DACC    TAPE or DISK ?      GP09286
         BM    OPNOTAP            Yes; supported                GP09286
         NI    WWORK,255-2        Change to plain output        GP09286
*OR-FAIL BNM   FAILDCB            No, not supported             GP09286
         SPACE 1
OPNOTAP  CLI   WWORK+1,2          UPDAT request?                GP09286
         BNE   OPNOTUP            No                            GP09286
         CLI   DWORK+2,UCB3DACC   DASD ?                        GP09286
         BNE   FAILDCB            No, not supported             GP09286
         SPACE 1
OPNOTUP  CLI   WWORK+1,4          INOUT or OUTIN ?              GP09286
         BL    OPNOTIO            No                            GP09286
         TM    DWORK+2,UCB3TAPE+UCB3DACC    TAPE or DISK ?      GP09286
         BNM   FAILDCB            No; not supported             GP09286
         SPACE 1
OPNOTIO  TM    WWORK,IOFEXCP      EXCP requested ?              GP09286
         BZ    OPFIXMD2                                         GP09286
         CLI   DWORK+2,UCB3TAPE   TAPE/CARTRIDGE device?        GP09286
         BE    OPFIXMD1           Yes; wonderful ?              GP09286
OPFIXMD0 NI    WWORK,255-IOFEXCP  Cancel EXCP request           GP09286
         B     OPFIXMD2                                         GP09286
OPFIXMD1 L     R0,BLKSIZE         GET USER'S SIZE               GP09286
         CH    R0,=H'32760'       NEED EXCP ?                   GP09286
         BNH   OPFIXMD0           NO; USE BSAM                  GP09286
         ST    R0,DWORK+4              Increase max size        GP09286
         ST    R0,LRECL           ALSO RECORD LENGTH            GP09286
         MVI   FILEMODE,2         FORCE RECFM=U                 GP09286
         SPACE 1
OPFIXMD2 IC    R4,WWORK           Fix up                        GP09286
OPFIXMOD STC   R4,WWORK           Save to storage               GP09286
         MVC   IOMFLAGS,WWORK     Save for duration             GP09286
         SPACE 1
*---------------------------------------------------------------------*
*   Do as much common code for input and output before splitting
*   Set mode flag in Open/Close list
*   Move BSAM, QSAM, or EXCP DCB to work area
*---------------------------------------------------------------------*
         STC   R4,OPENCLOS        Initialize MODE=24 OPEN/CLOSE list
         NI    OPENCLOS,X'07'        For now                    GP09286
*                  OPEN mode: IN OU UP AP IO OI                 GP09286
         TR    OPENCLOS(1),=X'80,8F,84,8E,83,86,0,0'            GP09286
         CLI   OPENCLOS,0         NOT SUPPORTED ?               GP09286
         BE    FAILDCB            FAIL REQUEST                  GP09286
         SPACE 1
         TM    WWORK,IOFEXCP      EXCP mode ?                   GP09286
         BZ    OPQRYBSM                                         GP09286
         MVC   ZDCBAREA(EXCPDCBL),EXCPDCB  Move DCB/IOB/CCW     GP09286
         LA    R15,TAPEIOB   FOR EASIER SETTINGS                GP09286
         USING IOBSTDRD,R15                                     GP09286
         MVI   IOBFLAG1,IOBDATCH+IOBCMDCH   COMMAND CHAINING IN USE
         MVI   IOBFLAG2,IOBRRT2                                 GP09286
         LA    R1,TAPEECB                                       GP09286
         ST    R1,IOBECBPT                                      GP09286
         LA    R1,TAPECCW                                       GP09286
         ST    R1,IOBSTART   CCW ADDRESS                        GP09286
         ST    R1,IOBRESTR   CCW ADDRESS                        GP09286
         LA    R1,TAPEDCB                                       GP09286
         ST    R1,IOBDCBPT   DCB                                GP09286
         LA    R1,TAPEIOB                                       GP09286
         STCM  R1,7,DCBIOBAA LINK IOB TO DCB FOR DUMP FORM.ING  GP09286
         LA    R0,1          SET BLOCK COUNT INCREMENT          GP09286
         STH   R0,IOBINCAM                                      GP09286
         DROP  R15                                              GP09286
         B     OPREPCOM                                         GP09286
         SPACE 1
OPQRYBSM TM    WWORK,IOFBLOCK     Block mode ?                  GP09286
         BNZ   OPREPBSM                                         GP09286
         TM    WWORK,X'01'        In or Out                     GP09286
*DEFUNCT BNZ   OPREPQSM
OPREPBSM MVC   ZDCBAREA(BSAMDCBL),BSAMDCB  Move DCB template to work
         TM    DWORK+2,UCB3DACC+UCB3TAPE    Tape or Disk ?      \
         BM    OPREPCOM           Either; keep RP,WP            \
         NC    DCBMACR(2),=AL1(DCBMRRD,DCBMRWRT) Strip Point    \
         B     OPREPCOM
         SPACE 1
OPREPQSM MVC   ZDCBAREA(QSAMDCBL),QSAMDCB
OPREPCOM MVC   DCBDDNAM,0(R3)                                   GP09286
         MVC   DEVINFO(8),DWORK   Check device type             GP09286
         ICM   R0,15,DEVINFO+4    Any ?
         BZ    FAILDCB            No DD card or ?               GP09286
         N     R4,=X'000000EF'    Reset block mode              GP09286
         TM    WWORK,IOFTERM      Terminal I/O?                 GP09286
         BNZ   OPFIXMOD                                         GP09286
         TM    WWORK,IOFBLOCK           Blocked I/O?            GP09286
         BZ    OPREPJFC                                         GP09286
         CLI   DEVINFO+2,UCB3UREC Unit record?                  GP09286
         BE    OPFIXMOD           Yes, may not block            GP09286
         SPACE 1
OPREPJFC LA    R14,JFCB
* EXIT TYPE 07 + 80 (END OF LIST INDICATOR)
         ICM   R14,B'1000',=X'87'
         ST    R14,DCBXLST+4                                    GP09286
         LA    R14,OCDCBEX        POINT TO DCB EXIT             GP09286
         ICM   R14,8,=X'05'         REQUEST IT                  GP09286
         ST    R14,DCBXLST        AND SET IT BACK               GP09286
         LA    R14,DCBXLST
         STCM  R14,B'0111',DCBEXLSA
         MVC   EOFR24(EOFRLEN),ENDFILE   Put EOF code below the line
         LA    R1,EOFR24
         STCM  R1,B'0111',DCBEODA
         RDJFCB ((R10)),MF=(E,OPENCLOS)  Read JOB File Control Blk
*---------------------------------------------------------------------*
*   If the caller did not request EXCP mode, but the user has BLKSIZE
*   greater than 32760 on TAPE, then we set the EXCP bit in R4 and
*   restart the OPEN. Otherwise MVS should fail?
*   The system fails explicit BLKSIZE in excess of 32760, so we cheat.
*   The NCP field is not otherwise honored, so if the value is 32 to
*   64 inclusive, we use that times 1024 as a value (max 65535)
*---------------------------------------------------------------------*
         CLI   DEVINFO+2,UCB3TAPE TAPE DEVICE?                  GP09286
         BNE   OPNOTBIG           NO                            GP09286
         TM    WWORK,IOFEXCP      USER REQUESTED EXCP ?         GP09286
         BNZ   OPVOLCNT           NOTHING TO DO                 GP09286
         CLI   JFCNCP,32          LESS THAN MIN ?               GP09286
         BL    OPNOTBIG           YES; IGNORE                   GP09286
         CLI   JFCNCP,65          NOT TOO HIGH ?                GP09286
         BH    OPNOTBIG           TOO BAD                       GP09286
*---------------------------------------------------------------------*
*   Clear DCB wrk area and force RECFM=U,BLKSIZE>32K                  *
*     and restart the OPEN processing                                 *
*---------------------------------------------------------------------*
         LR    R0,R10             Load output DCB area address  GP09286
         LA    R1,ZDCBLEN         Load output length            GP09286
         LA    R15,0              Pad of X'00'                  GP09286
         MVCL  R0,R14             Clear DCB area to zeroes      GP09286
         SR    R0,R0                                            GP09286
         ICM   R0,1,JFCNCP        NUMBER OF CHANNEL PROGRAMS    GP09286
         SLL   R0,10              *1024                         GP09286
         C     R0,=F'65535'       LARGER THAN CCW SUPPORTS?     GP09286
         BL    *+8                NO                            GP09286
         L     R0,=F'65535'       LOAD MAX SUPPORTED            GP09286
         ST    R0,BLKSIZE         MAKE NEW VALUES THE DEFAULT   GP09286
         ST    R0,LRECL           MAKE NEW VALUES THE DEFAULT   GP09286
         MVI   FILEMODE,2         USE RECFM=U                   GP09286
         LA    R0,IOFEXCP         GET EXCP OPTION               GP09286
         OR    R4,R0              ADD TO USER'S REQUEST         GP09286
         B     OPCURSE            AND RESTART THE OPEN          GP09286
         SPACE 1
OPVOLCNT SR    R1,R1                                            GP09286
         ICM   R1,1,JFCBVLCT      GET VOLUME COUNT FROM DD      GP09286
         BNZ   *+8                OK                            GP09286
         LA    R1,1               SET FOR ONE                   GP09286
         ST    R1,ZXCPVOLS        SAVE FOR EOV                  GP09286
         SPACE 1
OPNOTBIG CLI   DEVINFO+2,UCB3DACC   Is it a DASD device?
         BNE   OPNODSCB           No; no member name supported
*---------------------------------------------------------------------*
*   For a DASD resident file, get the format 1 DSCB                   *
*---------------------------------------------------------------------*
* CAMLST CAMLST SEARCH,DSNAME,VOLSER,DSCB+44
*
         L     R14,CAMDUM         Get CAMLST flags              GP09286
         LA    R15,JFCBDSNM       Load address of output data set name
         LA    R0,JFCBVOLS        Load addr. of output data set volser
         LA    R1,DS1FMTID        Load address of where to put DSCB
         STM   R14,R1,CAMLST      Complete CAMLST addresses     GP09286
         OBTAIN CAMLST            Read the VTOC record
         SPACE 1
* The member name may not be below the line, which may stuff up
* the "FIND" macro, so make sure it is in 24-bit memory.
OPNODSCB LTR   R9,R9              See if an address for the member name
         BZ    NOMEM              No member name, skip copying
         MVC   MEMBER24,0(R9)
         LA    R9,MEMBER24
         SPACE 1
***********************************************************************
*   Split READ and WRITE paths                                        *
*     Note that all references to DCBRECFM, DCBLRECL, and DCBBLKSI    *
*     have been replaced by ZRECFM, LRECL, and BLKSIZE for EXCP use.  *
***********************************************************************
NOMEM    TM    WWORK,1            See if OPEN input or output   GP09286
         BNZ   WRITING
*---------------------------------------------------------------------*
*
* READING
*   N.B. moved RDJFCB prior to member test to allow uniform OPEN and
*        other code. Makes debugging and maintenance easier
*
*---------------------------------------------------------------------*
         OI    JFCBTSDM,JFCNWRIT  Don't mess with DSCB
         CLI   DEVINFO+2,X'20' UCB3DACC   Is it a DASD device?
         BNE   OPENVSEQ           No; no member name supported
*---------------------------------------------------------------------*
* See if DSORG=PO but no member; use member from JFCB if one
*---------------------------------------------------------------------*
         TM    DS1DSORG,DS1DSGPO  See if DSORG=PO
         BZ    OPENVSEQ           Not PDS, don't read PDS directory
         TM    WWORK,X'07'   ANY NON-READ OPTION ?              GP09286
         BNZ   FAILDCB            NOT ALLOWED FOR PDS           GP09286
         LTR   R9,R9              See if an address for the member name
         BNZ   OPENMEM            Is member name - BPAM access
         TM    JFCBIND1,JFCPDS    See if a member name in JCL
         BZ    OPENDIR            No; read directory
         MVC   MEMBER24,JFCBELNM  Save the member name
         NI    JFCBIND1,255-JFCPDS    Reset it
         XC    JFCBELNM,JFCBELNM  Delete it
         LA    R9,MEMBER24        Force FIND to prevent 013 abend
         B     OPENMEM            Change DCB to BPAM PO
*---------------------------------------------------------------------*
* At this point, we have a PDS but no member name requested.
* Request must be to read the PDS directory
*---------------------------------------------------------------------*
OPENDIR  TM    OPENCLOS,X'0F'     Other than plain OPEN ?       GP09286
         BNZ   BADOPIN            No, fail (allow UPDAT later?) GP09286
         LA    R0,256             Set size for Directory BLock  GP09286
         STH   R0,DCBBLKSI        Set DCB BLKSIZE to 256        GP09286
         STH   R0,DCBLRECL        Set DCB LRECL to 256          GP09286
         ST    R0,LRECL                                         GP09286
         ST    R0,BLKSIZE                                       GP09286
         MVI   DCBRECFM,DCBRECF   Set DCB RECFM to RECFM=F (notU?)
         B     OPENIN
OPENMEM  MVI   DCBDSRG1,DCBDSGPO  Replace DCB DSORG=PS with PO
         OI    JFCBTSDM,JFCVSL    Force OPEN analysis of JFCB
         B     OPENIN
OPENVSEQ LTR   R9,R9              Member name for sequential?
         BNZ   BADOPIN            Yes, fail
         TM    IOMFLAGS,IOFEXCP   EXCP mode ?                   GP09286
         BNZ   OPENIN             YES                           GP09286
         OI    DCBOFLGS,DCBOFPPC  Allow unlike concatenation    GP09286
OPENIN   OPEN  MF=(E,OPENCLOS),TYPE=J  Open the data set
         TM    DCBOFLGS,DCBOFOPN  Did OPEN work?
         BZ    BADOPIN            OPEN failed, go return error code -37
         LTR   R9,R9              See if an address for the member name
         BZ    GETBUFF            No member name, skip finding it
*
         FIND  (R10),(R9),D       Point to the requested member
*
         LTR   R15,R15            See if member found
         BZ    GETBUFF            Member found, go get an input buffer
* If FIND return code not zero, process return and reason codes and
* return to caller with a negative return code.
         SLL   R15,8              Shift return code for reason code
         OR    R15,R0             Combine return code and reason code
         LR    R7,R15             Number to generate return and reason
         CLOSE MF=(E,OPENCLOS)    Close, FREEPOOL not needed
         B     FREEDCB                                          GP09286
BADOPIN  DS    0H
BADOPOUT DS    0H
FAILDCB  N     R4,=F'1'           Mask other option bits        GP09286
         LA    R7,37(R4,R4)       Preset OPEN error code        GP09286
FREEDCB  FREEMAIN RU,LV=ZDCBLEN,A=(R10),SP=SUBPOOL  Free DCB area
         LCR   R7,R7              Set return and reason code    GP09286
         B     RETURNOP           Go return to caller with negative RC
         SPACE 1
*---------------------------------------------------------------------*
*   Process for OUTPUT mode                                           *
*---------------------------------------------------------------------*
WRITING  LTR   R9,R9
         BZ    WNOMEM
         CLI   DEVINFO+2,X'20'    UCB3DACC
         BNE   BADOPOUT           Member name invalid
         TM    DS1DSORG,DS1DSGPO  See if DSORG=PO
         BZ    BADOPOUT           Is not PDS, fail request
         TM    WWORK,X'06'   ANY NON-RITE OPTION ?              GP09286
         BNZ   FAILDCB            NOT ALLOWED FOR PDS           GP09286
         MVC   JFCBELNM,0(R9)
         OI    JFCBIND1,JFCPDS
         OI    JFCBTSDM,JFCVSL    Just in case
         B     WNOMEM2            Go to move DCB info
WNOMEM   DS    0H
         TM    JFCBIND1,JFCPDS    See if a member name in JCL
         BO    WNOMEM2            Is member name, go to continue OPEN
* See if DSORG=PO but no member so OPEN output would destroy directory
         TM    DS1DSORG,DS1DSGPO  See if DSORG=PO
         BZ    WNOMEM2            Is not PDS, go OPEN
         WTO   'MVSSUPA - No member name for output PDS',ROUTCDE=11
         WTO   'MVSSUPA - Refuses to write over PDS directory',        C
               ROUTCDE=11
         ABEND 123                Abend without a dump
         SPACE 1
WNOMEM2  OPEN  MF=(E,OPENCLOS),TYPE=J
         TM    DCBOFLGS,DCBOFOPN  Did OPEN work?
         BZ    BADOPOUT           OPEN failed, go return error code -39
         SPACE 1
*---------------------------------------------------------------------*
*   Acquire one BLKSIZE buffer for our I/O; and one LRECL buffer      *
*   for use by caller for @@AWRITE, and us for @@AREAD.               *
*---------------------------------------------------------------------*
GETBUFF  L     R5,BLKSIZE         Load the input blocksize      GP09286
         LA    R6,4(,R5)          Add 4 in case RECFM=U buffer
         GETMAIN RU,LV=(R6),SP=SUBPOOL  Get input buffer storage
         ST    R1,ZBUFF1          Save for cleanup              GP09286
         ST    R6,ZBUFF1+4           ditto                      GP09286
         ST    R1,BUFFADDR        Save the buffer address for READ
         XC    0(4,R1),0(R1)      Clear the RECFM=U Record Desc. Word
         LA    R14,0(R5,R1)       Get end address               GP09286
         ST    R14,BUFFEND          for real                    GP09286
         SPACE 1
         L     R6,LRECL           Get record length             GP09286
         LA    R6,4(,R6)          Insurance                     GP09286
         GETMAIN RU,LV=(R6),SP=SUBPOOL  Get VBS build record area
         ST    R1,ZBUFF2          Save for cleanup              GP09286
         ST    R6,ZBUFF2+4           ditto                      GP09286
         LA    R14,4(,R1)                                       GP09286
         ST    R14,VBSADDR        Save the VBS read/user write  GP09286
         L     R5,PARM6           Get caller's BUFFER address   \
         ST    R14,0(,R5)         and return work address       \
         AR    R1,R6              Add size GETMAINed to find end
         ST    R1,VBSEND          Save address after VBS rec.build area
         B     DONEOPEN           Go return to caller with DCB info
         SPACE 1
         PUSH  USING                                            GP09286
*---------------------------------------------------------------------*
*   Establish ZDCBAREA for either @@AWRITE or @@AREAD processing to   *
*   a terminal, or SYSTSIN/SYSTERM in batch.                          *
*---------------------------------------------------------------------*
TERMOPEN MVC   IOMFLAGS,WWORK     Save for duration             GP09286
         NI    IOMFLAGS,IOFTERM+IOFOUT      IGNORE ALL OTHERS   GP09286
         MVC   ZDCBAREA(TERMDCBL),TERMDCB  Move DCB/IOB/CCW     GP09286
         MVC   ZIODDNM,0(R3)      DDNAME FOR DEBUGGING, ETC.    GP09286
         LTR   R9,R9              See if an address for the member name
         BNZ   FAILDCB            Yes; fail                     GP09286
         L     R14,PSATOLD-PSA    GET MY TCB                    GP09286
         USING TCB,R14                                          GP09286
         ICM   R15,15,TCBJSCB  LOOK FOR THE JSCB                GP09286
         BZ    FAILDCB       HUH ?                              GP09286
         USING IEZJSCB,R15                                      GP09286
         ICM   R15,15,JSCBPSCB  PSCB PRESENT ?                  GP09286
         BZ    FAILDCB       NO; NOT TSO                        GP09286
         L     R1,TCBFSA     GET FIRST SAVE AREA                GP09286
         N     R1,=X'00FFFFFF'    IN CASE AM31                  GP09286
         L     R1,24(,R1)         LOAD INVOCATION R1            GP09286
         USING CPPL,R1       DECLARE IT                         GP09286
         MVC   ZIOECT,CPPLECT                                   GP09286
         MVC   ZIOUPT,CPPLUPT                                   GP09286
         SPACE 1
         ICM   R6,15,BLKSIZE      Load the input blocksize      GP09286
         BP    *+12               Use it                        GP09286
         LA    R6,1024            Arbitrary non-zero size       GP09286
         ST    R6,BLKSIZE         Return it                     GP09286
         ST    R6,LRECL           Return it                     GP09286
         LA    R6,4(,R6)          Add 4 in case RECFM=U buffer  GP09286
         GETMAIN RU,LV=(R6),SP=SUBPOOL  Get input buffer storage
         ST    R1,ZBUFF2          Save for cleanup              GP09286
         ST    R6,ZBUFF2+4           ditto                      GP09286
         LA    R1,4(,R1)          Allow for RDW if not V        GP09286
         ST    R1,BUFFADDR        Save the buffer address for READ
         L     R5,PARM6           R5 points to ZBUFF2           GP09286
         ST    R1,0(,R5)          save the pointer              GP09286
         XC    0(4,R1),0(R1)      Clear the RECFM=U Record Desc. Word
         MVC   ZRECFM,FILEMODE    Requested format 0-2          GP09286
         NI    ZRECFM,3           Just in case                  GP09286
         TR    ZRECFM,=X'8040C0C0'    Change to F / V / U       GP09286
         POP   USING                                            GP09286
         SPACE 1
*   Lots of code tests DCBRECFM twice, to distinguish among F, V, and
*     U formats. We set the index byte to 0,4,8 to allow a single test
*     with a three-way branch.
DONEOPEN LR    R7,R10             Return DCB/file handle address
         LA    R0,8
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
         SRL   R0,2               Convert to caller's code      GP09286
         L     R5,PARM3           POINT TO RECFM
         ST    R0,0(,R5)          Pass either RECFM F or V to caller
         L     R1,LRECL           Load RECFM F or V max. record length
         ST    R1,0(,R8)          Return record length back to caller
         L     R5,PARM5           POINT TO BLKSIZE              GP09286
         L     R0,BLKSIZE         Load RECFM U maximum record length
         ST    R0,0(,R5)          Pass new BLKSIZE              GP09286
         L     R5,PARM2           POINT TO MODE                 GP09286
         MVC   3(1,R5),IOMFLAGS   Pass (updated) file mode back GP09286
* Finished with R5 now
*
RETURNOP FUNEXIT RC=(R7)          Return to caller              GP09286
*
* This is not executed directly, but copied into 24-bit storage
ENDFILE  LA    R6,1               Indicate @@AREAD reached end-of-file
         LNR   R6,R6              Make negative                 GP09286
         BR    R14                Return to instruction after the GET
EOFRLEN  EQU   *-ENDFILE
*
         LTORG ,
         SPACE 1
BSAMDCB  DCB   MACRF=(RP,WP),DSORG=PS,DDNAME=BSAMDCB, input and output *
               EXLST=OCDCBEX      JCB and DCB exits added later
BSAMDCBN EQU   *-BSAMDCB
READDUM  READ  NONE,              Read record Data Event Control Block C
               SF,                Read record Sequential Forward       C
               ,       (R10),     Read record DCB address              C
               ,       (R4),      Read record input buffer             C
               ,       (R5),      Read BLKSIZE or 256 for PDS.DirectoryC
               MF=L               List type MACRO
READLEN  EQU   *-READDUM
BSAMDCBL EQU   *-BSAMDCB
         SPACE 1
EXCPDCB  DCB   DDNAME=EXCPDCB,MACRF=E,DSORG=PS,REPOS=Y,BLKSIZE=0,      *
               DEVD=TA,EXLST=OCDCBEX,RECFM=U                    GP09286
         DC    8XL4'0'         CLEAR UNUSED SPACE               GP09286
         ORG   EXCPDCB+84    LEAVE ROOM FOR DCBLRECL            GP09286
         DC    F'0'          VOLUME COUNT                       GP09286
PATCCW   CCW   1,2-2,X'40',3-3                                  GP09286
         ORG   ,                                                GP09286
EXCPDCBL EQU   *-EXCPDCB     PATTERN TO MOVE                    GP09286
         SPACE 1
TERMDCB  PUTLINE MF=L        PATTERN FOR TERMINAL I/O           GP09286
TERMDCBL EQU   *-TERMDCB     SIZE OF IOPL                       GP09286
         SPACE 1
F65536   DC    F'65536'           Maximum VBS record GETMAIN length
*
* QSAMDCB changes depending on whether we are in LOCATE mode or
* MOVE mode
QSAMDCB  DCB   MACRF=P&OUTM.M,DSORG=PS,DDNAME=QSAMDCB           GP09286
QSAMDCBL EQU   *-QSAMDCB
*
*
* CAMDUM CAMLST SEARCH,DSNAME,VOLSER,DSCB+44
CAMDUM   CAMLST SEARCH,*-*,*-*,*-*
CAMLEN   EQU   *-CAMDUM           Length of CAMLST Template
         POP   USING                                            GP09286
         SPACE 1
*---------------------------------------------------------------------*
*   Expand OPEN options for reference                                 *
*---------------------------------------------------------------------*
ADHOC    DSECT ,                                                GP09286
OPENREF  OPEN  (BSAMDCB,INPUT),MF=L    QSAM, BSAM, any DEVTYPE  GP09286
         OPEN  (BSAMDCB,OUTPUT),MF=L   QSAM, BSAM, any DEVTYPE  GP09286
         OPEN  (BSAMDCB,UPDAT),MF=L    QSAM, BSAM, DASD         GP09286
         OPEN  (BSAMDCB,EXTEND),MF=L   QSAM, BSAM, DASD, TAPE   GP09286
         OPEN  (BSAMDCB,INOUT),MF=L          BSAM, DASD, TAPE   GP09286
         OPEN  (BSAMDCB,OUTINX),MF=L         BSAM, DASD, TAPE   GP09286
         OPEN  (BSAMDCB,OUTIN),MF=L          BSAM, DASD, TAPE   GP09286
         SPACE 1
PARMSECT DSECT ,             MAP CALL PARM                      GP09286
PARM1    DS    A             FIRST PARM                         GP09286
PARM2    DS    A              NEXT PARM                         GP09286
PARM3    DS    A              NEXT PARM                         GP09286
PARM4    DS    A              NEXT PARM                         GP09286
PARM5    DS    A              NEXT PARM                         GP09286
PARM6    DS    A              NEXT PARM                         GP09286
PARM7    DS    A              NEXT PARM                         GP09286
PARM8    DS    A              NEXT PARM                         GP09286
MVSSUPA  CSECT ,                                                GP09286
         SPACE 1
         ORG   CAMDUM+4           Don't need rest
         SPACE 2
***********************************************************************
**                                                                   **
**   OPEN DCB EXIT - if RECFM, LRECL, BLKSIZE preset, no change      **
**                   for PDS directory read, F, 256, 256 are preset. **
**   a) device is unit record - default U, device size, device size  **
**   b) all others - default to values passed to AOPEN               **
**                                                                   **
**   For FB, if LRECL > BLKSIZE, make LRECL=BLKSIZE                  **
**   For VB, if LRECL+3 > BLKSIZE, set spanned                       **
**                                                                   **
***********************************************************************
         PUSH  USING                                            GP09286
         DROP  ,                                                GP09286
         USING OCDCBEX,R15                                      GP09286
         USING IHADCB,R1     DECLARE OUR DCB WORK SPACE         GP09286
OCDCBEX  LR    R11,R1        SAVE DCB ADDRESS AND OPEN FLAGS    GP09286
         N     R1,=X'00FFFFFF'   NO 0C4 ON DCB ACCESS IF AM31   GP09286
         TM    IOPFLAGS,IOFDCBEX  Been here before ?            GP09286
         BZ    OCDCBX1                                          GP09286
         OI    IOPFLAGS,IOFCONCT  Set unlike concatenation      GP09286
         OI    DCBOFLGS,DCBOFPPC  Keep them coming              GP09286
OCDCBX1  OI    IOPFLAGS,IOFDCBEX  Show exit entered             GP09286
         SR    R2,R2         FOR POSSIBLE DIVIDE (FB)           GP09286
         SR    R3,R3                                            GP09286
         ICM   R3,3,DCBBLKSI   GET CURRENT BLOCK SIZE           GP09286
         SR    R4,R4         FOR POSSIBLE LRECL=X               GP09286
         ICM   R4,3,DCBLRECL GET CURRENT RECORD LENGTH          GP09286
         NI    FILEMODE,3    MASK FILE MODE                     GP09286
         MVC   ZRECFM,FILEMODE   GET OPTION BITS                GP09286
         TR    ZRECFM,=X'90,50,C0,C0'  0-FB  1-VB  2-U          GP09286
         TM    DCBRECFM,DCBRECLA  ANY RECORD FORMAT SPECIFIED?  GP09286
         BNZ   OCDCBFH       YES                                GP09286
         CLI   DEVINFO+2,X'08'    UNIT RECORD?                  GP09286
         BNE   OCDCBFM       NO; USE OVERRIDE                   GP09286
OCDCBFU  CLI   FILEMODE,0         DID USER REQUEST FB?          GP09286
         BE    OCDCBFM            YES; USE IT                   GP09286
         OI    DCBRECFM,DCBRECU   SET U FOR READER/PUNCH/PRINTER
         B     OCDCBFH                                          GP09286
OCDCBFM  MVC   DCBRECFM,ZRECFM                                  GP09286
OCDCBFH  LTR   R4,R4                                            GP09286
         BNZ   OCDCBLH       HAVE A RECORD LENGTH               GP09286
         L     R4,DEVINFO+4       SET DEVICE SIZE FOR UNIT RECORD
         CLI   DEVINFO+2,X'08'    UNIT RECORD?                  GP09286
         BE    OCDCBLH       YES; USE IT                        GP09286
*   REQUIRES CALLER TO SET LRECL=BLKSIZE FOR RECFM=U DEFAULT    GP09286
         ICM   R4,15,LRECL   SET LRECL=PREFERRED BLOCK SIZE     GP09286
         BNZ   *+8                                              GP09286
         L     R4,DEVINFO+4  ELSE USE DEVICE MAX                GP09286
         IC    R5,DCBRECFM   GET RECFM                          GP09286
         N     R5,=X'000000C0'  RETAIN ONLY D,F,U,V             GP09286
         SRL   R5,6          CHANGE TO 0-D 1-V 2-F 3-U          GP09286
         MH    R5,=H'3'      PREPARE INDEX                      GP09286
         SR    R6,R6                                            GP09286
         IC    R6,FILEMODE   GET USER'S VALUE                   GP09286
         AR    R5,R6         DCB VS. DFLT ARRAY                 GP09286
*     DCB RECFM:       --D--- --V--- --F--- --U---              GP09286
*     FILE MODE:       F V  U F V  U F  V U F  V U              GP09286
         LA    R6,=AL1(4,0,-4,4,0,-4,0,-4,0,0,-4,0)  LRECL ADJUST
         AR    R6,R5         POINT TO ENTRY                     GP09286
         ICM   R5,8,0(R6)    LOAD IT                            GP09286
         SRA   R5,24         SHIFT WITH SIGN EXTENSION          GP09286
         AR    R4,R5         NEW LRECL                          GP09286
         SPACE 1                                                GP09286
*   NOW CHECK BLOCK SIZE                                        GP09286
OCDCBLH  LTR   R3,R3         ANY ?                              GP09286
         BNZ   *+8           YES                                GP09286
         ICM   R3,15,BLKSIZE SET OUR PREFERRED SIZE             GP09286
         BNZ   *+8           OK                                 GP09286
         L     R3,DEVINFO+4  SET NON-ZERO                       GP09286
         C     R3,DEVINFO+4  LEGAL ?                            GP09286
         BNH   *+8                                              GP09286
         L     R3,DEVINFO+4  NO; SHORTEN                        GP09286
         TM    DCBRECFM,DCBRECU   U?                            GP09286
         BO    OCDCBBU       YES                                GP09286
         TM    DCBRECFM,DCBRECF   FIXED ?                       GP09286
         BZ    OCDCBBV       NO; CHECK VAR                      GP09286
         DR    R2,R4                                            GP09286
         CH    R3,=H'1'      DID IT FIT ?                       GP09286
         BE    OCDCBBF       BARELY                             GP09286
         BH    OCDCBBB       ELSE LEAVE BLOCKED                 GP09286
         LA    R3,1          SET ONE RECORD MINIMUM             GP09286
OCDCBBF  NI    DCBRECFM,255-DCBRECBR   BLOCKING NOT NEEDED      GP09286
OCDCBBB  MR    R2,R4         BLOCK SIZE NOW MULTIPLE OF LRECL   GP09286
         B     OCDCBXX       AND GET OUT                        GP09286
*   VARIABLE                                                    GP09286
OCDCBBV  LA    R5,4(,R4)     LRECL+4                            GP09286
         CR    R5,R3         WILL IT FIT ?                      GP09286
         BNH   *+8           YES                                GP09286
         OI    DCBRECFM,DCBRECSB  SET SPANNED                   GP09286
         B     OCDCBXX       AND EXIT                           GP09286
*   UNDEFINED                                                   GP09286
OCDCBBU  LR    R4,R3         FOR NEATNESS, SET LRECL = BLOCK SIZE
*   EXEUNT  (Save DCB options for EXCP compatibility in main code)
OCDCBXX  STH   R3,DCBBLKSI   UPDATE POSSIBLY CHANGED BLOCK SIZE GP09286
         STH   R4,DCBLRECL     AND RECORD LENGTH                GP09286
         ST    R3,BLKSIZE    UPDATE POSSIBLY CHANGED BLOCK SIZE GP09286
         ST    R4,LRECL        AND RECORD LENGTH                GP09286
         MVC   ZRECFM,DCBRECFM    DITTO                         GP09286
         BR    R14           RETURN TO OPEN                     GP09286
         POP   USING                                            GP09286
         SPACE 2
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  ALINE - See whether any more input is available
*     R15=0 EOF     R15=1 More data available
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
@@ALINE  FUNHEAD IO=YES,AM=YES,SAVE=(WORKAREA,WORKLEN,SUBPOOL)  GP09286
         FIXWRITE ,                                             GP09286
         TM    IOMFLAGS,IOFTERM   Terminal Input?               GP09286
         BNZ   ALINEYES             Always one more?            GP09286
         LA    R3,KEPTREC                                       GP09286
         LA    R4,KEPTREC+4                                     GP09286
         STM   R2,R4,DWORK   BUILD PARM LIST                    GP09286
         LA    R15,@@AREAD                                      GP09286
         LA    R1,DWORK                                         GP09286
         BALR  R14,R15       GET NEXT RECORD                    GP09286
         SR    R15,R15       SET EOF FLAG                       GP09286
         LTR   R6,R6         HIT EOF ?                          GP09286
         BM    ALINEX        YES; RETURN ZERO                   GP09286
         OI    IOPFLAGS,IOFKEPT   SHOW WE'RE KEEPING A RECORD   GP09286
ALINEYES LA    R15,1         ELSE RETURN ONE                    GP09286
ALINEX   FUNEXIT RC=(R15)                                       GP09286
         SPACE 2
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  AREAD - Read from an open data set
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
@@AREAD  FUNHEAD IO=YES,AM=YES,SAVE=SAVEADCB   READ | GET       GP09286
         L     R3,4(,R1)  R3 points to where to store record pointer
         L     R4,8(,R1)  R4 points to where to store record length
         SR    R0,R0                                            GP09286
         ST    R0,0(,R3)          Return null in case of EOF    GP09286
         ST    R0,0(,R4)          Return null in case of EOF    GP09286
         FIXWRITE ,               For OUTIN request             GP09286
         L     R6,=F'-1'          Prepare for EOF signal        GP09286
         TM    IOPFLAGS,IOFKEPT   Saved record ?                GP09286
         BZ    READQEOF           No; check for EOF             GP09286
         LM    R8,R9,KEPTREC      Get prior address & length    GP09286
         ST    R8,0(,R3)          Set address                   GP09286
         ST    R9,0(,R4)            and length                  GP09286
         XC    KEPTREC(8),KEPTREC Reset record info             GP09286
         NI    IOPFLAGS,IOFKEPT   Reset flag                    GP09286
         SR    R6,R6              No EOF                        GP09286
         B     READEXIT                                         GP09286
         SPACE 1
READQEOF TM    IOPFLAGS,IOFLEOF   Prior EOF ?
         BNZ   READEXIT           Yes; don't abend
         TM    IOMFLAGS,IOFTERM   GETLIN request?               GP09286
         BNZ   TGETREAD           Yes                           GP09286
*   Return here for end-of-block or unlike concatenation
*
REREAD   SLR   R6,R6              Clear default end-of-file indicator
         ICM   R8,B'1111',BUFFCURR  Load address of next record
         BNZ   DEBLOCK            Block in memory, go de-block it
         L     R8,BUFFADDR        Load address of input buffer
         L     R9,BLKSIZE         Load block size to read
         CLI   RECFMIX,4          RECFM=Vxx ?                   GP09286
         BE    READ               No, deblock                   GP09286
         LA    R8,4(,R8)          Room for fake RDW             GP09286
READ     SAM24 ,                  For old code                  GP09286
         TM    IOMFLAGS,IOFEXCP   EXCP mode?                    GP09286
         BZ    READBSAM           No, use BSAM                  GP09286
*---------------------------------------------------------------------*
*   EXCP read                                                         *
*---------------------------------------------------------------------*
READEXCP STCM  R8,7,TAPECCW+1     Read buffer                   GP09286
         STH   R9,TAPECCW+6         max length                  GP09286
         MVI   TAPECCW,2          READ                          GP09286
         MVI   TAPECCW+4,X'20'    SILI bit                      GP09286
         EXCP  TAPEIOB            Read                          GP09286
         WAIT  ECB=TAPEECB        wait for completion           GP09286
         TM    TAPEECB,X'7F'      Good ?                        GP09286
         BO    EXRDOK             Yes; calculate input length   GP09286
         CLI   TAPEECB,X'41'      Tape Mark read ?              GP09286
         BNE   EXRDBAD            NO                            GP09286
         CLM   R9,3,IOBCSW+5-IOBSTDRD+TAPEIOB  All unread?      GP09286
         BNE   EXRDBAD            NO                            GP09286
         L     R1,DCBBLKCT                                      GP09286
         BCTR  R1,0                                             GP09286
         ST    R1,DCBBLKCT        allow for tape mark           GP09286
         OI    DCBOFLGS,X'04'     Set tape mark found           GP09286
         L     R0,ZXCPVOLS        Get current volume count      GP09286
         SH    R0,=H'1'           Just processed one            GP09286
         ST    R0,ZXCPVOLS                                      GP09286
         BNP   READEOD            None left - take End File     GP09286
         EOV   TAPEDCB            switch volumes                GP09286
         B     READEXCP           and restart                   GP09286
         SPACE 1
EXRDBAD  ABEND 001,DUMP           bad way to show error?        GP09286
         SPACE 1
EXRDOK   SR    R0,R0                                            GP09286
         ICM   R0,3,IOBCSW+5-IOBSTDRD+TAPEIOB                   GP09286
         SR    R9,R0         LENGTH READ                        GP09286
         BNP   BADBLOCK      NONE ?
         AMUSE ,                  Restore caller's mode         GP09286
         LTR   R6,R6              See if end of input data set  GP09286
         BM    READEOD            Is end, go return to caller   GP09286
         B     POSTREAD           Go to common code             GP09286
         SPACE 1
*---------------------------------------------------------------------*
*   BSAM read                                                         *
*---------------------------------------------------------------------*
READBSAM SR    R6,R6              Reset EOF flag                GP09286
         SAM24 ,                  Get low                       GP09296
         READ  DECB,              Read record Data Event Control Block C
               SF,                Read record Sequential Forward       C
               (R10),             Read record DCB address              C
               (R8),              Read record input buffer             C
               (R9),              Read BLKSIZE or 256 for PDS.DirectoryC
               MF=E               Execute a MF=L MACRO
*                                 If EOF, R6 will be set to F'-1'
         CHECK DECB               Wait for READ to complete
         TM    IOPFLAGS,IOFCONCT  Did we hit concatenation?     GP09286
         BZ    READUSAM           No; restore user's AM         GP09286
         NI    IOPFLAGS,255-IOFCONCT   Reset for next time      GP09286
         ICM   R6,8,DCBRECFM                                    GP09286
         SRL   R6,24+2            Isolate top two bits          GP09286
         STC   R6,RECFMIX         Store                         GP09286
         TR    RECFMIX,=X'01010002'    Filemode D, V, F, U      GP09286
         MVC   LRECL+2(2),DCBLRECL  Also return record length   GP09286
         MVC   ZRECFM,DCBRECFM    and format                    GP09286
         B     READBSAM           Reissue the READ              GP09286
         SPACE 1
READUSAM AMUSE ,                  Restore caller's mode         GP09286
         LTR   R6,R6              See if end of input data set
         BM    READEOD            Is end, go return to caller
         L     R14,DECB+16    DECIOBPT                          GP09286
         USING IOBSTDRD,R14       Give assembler IOB base
         SLR   R1,R1              Clear residual amount work register
         ICM   R1,B'0011',IOBCSW+5  Load residual count
         DROP  R14                Don't need IOB address base anymore
         SR    R9,R1              Provisionally return blocklen GP09286
         SPACE 1
POSTREAD TM    IOMFLAGS,IOFBLOCK  Block mode ?                  GP09286
         BNZ   POSTBLOK           Yes; process as such          GP09286
         TM    ZRECFM,DCBRECU     Also exit for U               GP09286
         BNO   POSTREED                                         GP09286
POSTBLOK ST    R8,0(,R3)          Return address to user        GP09286
         ST    R9,0(,R4)          Return length to user         GP09286
         STM   R8,R9,KEPTREC      Remember record info          GP09286
         XC    BUFFCURR,BUFFCURR  Show READ required next call  GP09286
         B     READEXIT                                         GP09286
POSTREED CLI   RECFMIX,4          See if RECFM=V                GP09286
         BNE   EXRDNOTV           Is RECFM=U or F, so not RECFM=V
         ICM   R9,3,0(R8)         Get presumed block length     GP09286
         C     R9,BLKSIZE         Valid?                        GP09286
         BH    BADBLOCK           No                            GP09286
         ICM   R0,3,2(R8)         Garbage in BDW?               GP09286
         BNZ   BADBLOCK           Yes; fail                     GP09286
         B     EXRDCOM                                          GP09286
EXRDNOTV LA    R0,4(,R9)          Fake length                   GP09286
         SH    R8,=H'4'           Space to fake RDW             GP09286
         STH   R0,0(0,R8)         Fake RDW                      GP09286
         LA    R9,4(,R9)          Up for fake RDW (F/U)         GP09286
EXRDCOM  LA    R8,4(,R8)          Bump buffer address past BDW  GP09286
         SH    R9,=H'4'             and adjust length to match  GP09286
         BNP   BADBLOCK           Oops                          GP09286
         ST    R8,BUFFCURR        Indicate data available       GP09286
         ST    R8,0(,R3)          Return address to user        GP09286
         ST    R9,0(,R4)          Return length to user         GP09286
         STM   R8,R9,KEPTREC      Remember record info          GP09286
         LA    R7,0(R9,R8)        End address + 1               GP09286
         ST    R7,BUFFEND         Save end                      GP09286
         SPACE 1
         TM    IOMFLAGS,IOFBLOCK   Block mode?                  GP09286
         BNZ   READEXIT           Yes; exit                     GP09286
         TM    ZRECFM,DCBRECU     Also exit for U               GP09286
         BO    READEXIT                                         GP09286
*NEXT*   B     DEBLOCK            Else deblock                  GP09286
         SPACE 1
*        R8 has address of current record
DEBLOCK  CLI   RECFMIX,4          Is data set RECFM=U
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
         C     R9,LRECL      VALID LENGTH ?
         BH    BADBLOCK      NO
         LA    R7,0(R9,R8)   SET ADDRESS OF LAST BYTE +1
         C     R7,BUFFEND    WILL IT FIT INTO BUFFER ?
         BL    DEBVCURR      LOW - LEAVE IT
         BH    BADBLOCK      NO; FAIL
         SR    R7,R7         PRESET FOR BLOCK DONE
DEBVCURR ST    R7,BUFFCURR        for recursion
         TM    3(R8),X'FF'   CLEAN RDW ?
         BNZ   BADBLOCK
         TM    IOPFLAGS,IOFLSDW   WAS PREVIOUS RECORD DONE ?
         BO    DEBVAPND           NO
         LH    R0,0(,R8)          Provisional length if simple  GP09286
         ST    R0,0(,R4)          Return length                 GP09286
         ST    R0,KEPTREC+4       Remember record info          GP09286
         CLI   2(R8),1            What is this?
         BL    SETCURR            Simple record
         BH    BADBLOCK           Not=1; have a sequence error
         OI    IOPFLAGS,IOFLSDW   Starting a new segment
         L     R2,VBSADDR         Get start of buffer           GP09286
         MVC   0(4,R2),=X'00040000'   Preset null record
         B     DEBVMOVE           And move this
DEBVAPND CLI   2(R8),3            IS THIS A MIDDLE SEGMENT ?
         BE    DEBVMOVE           YES, PUT IT OUT
         CLI   2(R8),2            IS THIS THE LAST SEGMENT ?
         BNE   BADBLOCK           No; bad segment sequence
         NI    IOPFLAGS,255-IOFLSDW  INDICATE RECORD COMPLETE
DEBVMOVE L     R2,VBSADDR         Get segment assembly area
         SR    R1,R1              Never trust anyone            GP09286
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
         SR    R0,R0                                            GP09286
         ICM   R0,3,0(R8)         Provisional length if simple  GP09286
         ST    R0,0(,R4)          Return length                 GP09286
         ST    R0,KEPTREC+4       Remember record info          GP09286
         B     SETCURR            Done
         SPACE 2
* If RECFM=FB, bump address by lrecl
*        R8 has address of current record
DEBLOCKF L     R7,LRECL           Load RECFM=F DCB LRECL
         ST    R7,0(,R4)          Return length                 GP09286
         ST    R7,KEPTREC+4       Remember record info          GP09286
         AR    R7,R8              Find the next record address
* If address=BUFFEND, zero BUFFCURR
SETCURR  CL    R7,BUFFEND         Is it off end of block?
         BL    SETCURS            Is not off, go store it
         SR    R7,R7              Clear the next record address
SETCURS  ST    R7,BUFFCURR        Store the next record address
         ST    R8,0(,R3)          Store record address for caller
         ST    R8,KEPTREC         Remember record info          GP09286
         B     READEXIT
         SPACE 1
TGETREAD L     R6,ZIOECT          RESTORE ECT ADDRESS           GP09286
         L     R7,ZIOUPT          RESTORE UPT ADDRESS           GP09286
         MVI   ZGETLINE+2,X'80'   EXPECTED FLAG                 GP09286
         GETLINE PARM=ZGETLINE,ECT=(R6),UPT=(R7),ECB=ZIOECB,    GP09286*
               MF=(E,ZIOPL)                                     GP09286
         LR    R6,R15             COPY RETURN CODE              GP09286
         CH    R6,=H'16'          HIT BARRIER ?                 GP09286
         BE    READEOD2           YES; EOF, BUT ALLOW READS     GP09286
         CH    R6,=H'8'           SERIOUS ?                     GP09286
         BNL   READEXNG           ATTENTION INTERRUPT OR WORSE  GP09286
         L     R1,ZGETLINE+4      GET INPUT LINE                GP09286
*---------------------------------------------------------------------*
*   MVS 3.8 undocumented behavior: at end of input in batch execution,*
*   returns text of 'END' instead of return code 16. Needs DOC fix    *
*---------------------------------------------------------------------*
         CLC   =X'00070000C5D5C4',0(R1)  Undocumented EOF?      GP09286
         BE    READEOD2           YES; QUIT                     GP09286
         L     R6,BUFFADDR        GET INPUT BUFFER              GP09286
         LR    R8,R1              INPUT LINE W/RDW              GP09286
         LH    R9,0(,R1)          GET LENGTH                    GP09286
         LR    R7,R9               FOR V, IN LEN = OUT LEN      GP09286
         CLI   RECFMIX,4          RECFM=V ?                     GP09286
         BE    TGETHAVE           YES                           GP09286
         BL    TGETSKPF                                         GP09286
         SH    R7,=H'4'           ALLOW FOR RDW                 GP09286
         B     TGETSKPV                                         GP09286
TGETSKPF L     R7,LRECL             FULL SIZE IF F              GP09286
TGETSKPV LA    R8,4(,R8)          SKIP RDW                      GP09286
         SH    R9,=H'4'           LENGTH SANS RDW               GP09286
TGETHAVE ST    R6,0(,R3)          RETURN ADDRESS                GP09286
         ST    R7,0(,R4)            AND LENGTH                  GP09286
         STM   R6,R7,KEPTREC      Remember record info          GP09286
         ICM   R9,8,=C' '           BLANK FILL                  GP09286
         MVCL  R6,R8              PRESERVE IT FOR USER          GP09286
         LH    R0,0(,R1)          GET LENGTH                    GP09286
*ABND30A FREEMAIN R,LV=(0),A=(1)  FREE SYSTEM BUFFER            GP09286
         SR    R6,R6              NO EOF                        GP09286
         B     READEXIT           TAKE NORMAL EXIT              GP09286
         SPACE 1
READEOD  OI    IOPFLAGS,IOFLEOF   Remember that we hit EOF
READEOD2 XC    KEPTREC(8),KEPTREC Clear saved record info       GP09286
         LA    R6,1                                             GP09286
READEXNG LNR   R6,R6              Signal EOF                    GP09286
READEXIT FUNEXIT RC=(R6) =1-EOF   Return to caller              GP09286
*
BADBLOCK WTO   'MVSSUPA - @@AREAD - problem processing RECFM=V(bs) file*
               ',ROUTCDE=11       Send to programmer and listing
         ABEND 1234,DUMP          Abend U1234 and allow a dump
*
         LTORG ,                  In case someone adds literals
         SPACE 2
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  AWRITE - Write to an open data set
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
@@AWRITE FUNHEAD IO=YES,AM=YES,SAVE=SAVEADCB   WRITE | PUT      GP09286
         LR    R11,R1             SAVE PARM LIST                GP09286
WRITMORE NI    IOPFLAGS,255-IOFCURSE   RESET RECURSION          GP09286
         L     R4,4(,R11)         R4 points to the record address
         L     R4,0(,R4)          Get record address            GP09286
         L     R5,8(,R11)         R5 points to length of data to write
         L     R5,0(,R5)          Length of data to write       GP09286
         TM    IOMFLAGS,IOFTERM   PUTLIN request?               GP09286
         BNZ   TPUTWRIT           Yes                           GP09286
*
         TM    IOMFLAGS,IOFBLOCK  Block mode?                   GP09286
         BNZ   WRITBLK            Yes                           GP09286
         CLI   OPENCLOS,X'84'     Running in update mode ?      GP09286
         BNE   WRITENEW           No                            GP09286
         LM    R2,R3,KEPTREC      Get last record returned      GP09286
         LTR   R3,R3              Any?                          GP09286
         BNP   WRITEEX            No; ignore (or abend?)        GP09286
         CLI   RECFMIX,4          RECFM=V...                    GP09286
         BNE   WRITUPMV           NO                            GP09286
         LA    R0,4               ADJUST FOR RDW                GP09286
         AR    R2,R0              KEEP OLD RDW                  GP09286
         SR    R3,R0              ADJUST REPLACE LENGTH         GP09286
         AR    R4,R0              SKIP OVER USER'S RDW          GP09286
         SR    R5,R0              ADJUST LENGTH                 GP09286
WRITUPMV MVCL  R2,R4              REPLACE DATA IN BUFFER        GP09286
         OI    IOPFLAGS,IOFLDATA  SHOW DATA IN BUFFER           GP09286
         B     WRITEEX            REWRITE ON NEXT READ OR CLOSE GP09286
         SPACE 1                                                GP09286
WRITENEW CLI   RECFMIX,4          V-FORMAT ?                    GP09286
         BH    WRITBLK            U - WRITE BLOCK AS IS         GP09286
         BL    WRITEFIX           F - ADD RECORD TO BLOCK       GP09286
         CH    R5,0(,R4)          RDW LENGTH = REQUESTED LEN?   GP09286
         BNE   WRITEBAD           NO; FAIL                      GP09286
         L     R8,BUFFADDR        GET BUFFER                    GP09286
         ICM   R6,15,BUFFCURR     Get next record address       GP09286
         BNZ   WRITEVAT                                         GP09286
         LA    R0,4                                             GP09286
         STH   R0,0(,R8)          BUILD BDW                     GP09286
         LA    R6,4(,R8)          SET TO FIRST RECORD POSITION  GP09286
WRITEVAT L     R9,BUFFEND         GET BUFFER END                GP09286
         SR    R9,R6              LESS CURRENT POSITION         GP09286
         TM    ZRECFM,DCBRECSB    SPANNED?                      GP09286
         BZ    WRITEVAR           NO; ROUTINE VARIABLE WRITE    GP09286
         LA    R1,4(,R5)          GET RECORD + BDW LENGTH       GP09286
         C     R1,LRECL           VALID SIZE?                   GP09286
         BH    WRITEBAD           NO; TAKE A DIVE               GP09286
         TM    IOPFLAGS,IOFLSDW   CONTINUATION ?                GP09286
         BNZ   WRITEVAW           YES; DO HERE                  GP09286
         CR    R5,R9              WILL IT FIT AS IS?            GP09286
         BNH   WRITEVAS           YES; DON'T NEED TO SPLIT      GP09286
WRITEVAW CH    R9,=H'5'           AT LEAST FIVE BYTES LEFT ?    GP09286
         BL    WRITEVNU           NO; WRITE THIS BLOCK; RETRY   GP09286
         LR    R3,R6              SAVE START ADDRESS            GP09286
         LR    R7,R9              COPY LENGTH                   GP09286
         CR    R7,R5              ROOM FOR ENTIRE SEGMENT ?     GP09286
         BL    *+4+2              NO                            GP09286
         LR    R7,R5              USE ONLY WHAT'S AVAILABLE     GP09286
         MVCL  R6,R4              COPY RDW + DATA               GP09286
         ST    R6,BUFFCURR        UPDATE NEXT AVAILABLE         GP09286
         SR    R6,R8              LESS START                    GP09286
         STH   R6,0(,R8)          UPDATE BDW                    GP09286
         STH   R9,0(,R3)          FIX RDW LENGTH                GP09286
         MVC   2(2,R3),=X'0100'   SET FLAGS FOR START SEGMENT   GP09286
         TM    IOPFLAGS,IOFLSDW   DID START ?                   GP09286
         BZ    *+4+6              NO; FIRST SEGMENT             GP09286
         MVI   2(R3),3            SHOW MIDDLE SEGMENT           GP09286
         LTR   R5,R5              DID WE FINISH THE RECORD ?    GP09286
         BP    WRITEWAY           NO                            GP09286
         MVI   2(R3),2            SHOW LAST SEGMENT             GP09286
         NI    IOPFLAGS,255-IOFLSDW-IOFCURSE  RCD COMPLETE      GP09286
         OI    IOPFLAGS,IOFLDATA  SHOW WRITE DATA IN BUFFER     GP09286
         B     WRITEEX            DONE                          GP09286
WRITEWAY SH    R9,=H'4'           ALLOW FOR EXTRA RDW           GP09286
         AR    R4,R9                                            GP09286
         SR    R5,R9                                            GP09286
         STM   R4,R5,KEPTREC      MAKE FAKE PARM LIST           GP09286
         LA    R11,KEPTREC-4      SET FOR RECURSION             GP09286
         OI    IOPFLAGS,IOFLSDW   SHOW RECORD INCOMPLETE        GP09286
         B     WRITEVNU           GO FOR MORE                   GP09286
         SPACE 1
WRITEVAR LA    R1,4(,R5)          GET RECORD + BDW LENGTH       GP09286
         C     R1,BLKSIZE         VALID SIZE?                   GP09286
         BH    WRITEBAD           NO; TAKE A DIVE               GP09286
         L     R9,BUFFEND         GET BUFFER END                GP09286
         SR    R9,R6              LESS CURRENT POSITION         GP09286
         CR    R5,R9              WILL IT FIT ?                 GP09286
         BH    WRITEVNU           NO; WRITE NOW AND RECURSE     GP09286
WRITEVAS LR    R7,R5              IN LENGTH = MOVE LENGTH       GP09286
         MVCL  R6,R4              MOVE USER'S RECORD            GP09286
         ST    R6,BUFFCURR        UPDATE NEXT AVAILABLE         GP09286
         SR    R6,R8              LESS START                    GP09286
         STH   R6,0(,R8)          UPDATE BDW                    GP09286
         OI    IOPFLAGS,IOFLDATA  SHOW WRITE DATA IN BUFFER     GP09286
         B     WRITEEX                                          GP09286
         SPACE 1
WRITEVNU OI    IOPFLAGS,IOFCURSE  SET RECURSION REQUEST         GP09286
         B     WRITPREP           SET ADDRESS/LENGTH TO WRITE   GP09286
         SPACE 1
WRITEBAD ABEND 002,DUMP           INVALID REQUEST               GP09286
         SPACE 1
WRITEFIX ICM   R6,15,BUFFCURR     Get next available record     GP09286
         BNZ   WRITEFAP           Not first                     GP09286
         L     R6,BUFFADDR        Get buffer start              GP09286
WRITEFAP L     R7,LRECL           Record length                 GP09286
         ICM   R5,8,=C' '         Request blank padding         GP09286
         MVCL  R6,R4              Copy record to buffer         GP09286
         ST    R6,BUFFCURR        Update new record address     GP09286
         OI    IOPFLAGS,IOFLDATA  SHOW DATA IN BUFFER           GP09297
         C     R6,BUFFEND         Room for more ?               GP09286
         BL    WRITEEX            YES; RETURN                   GP09286
WRITPREP L     R4,BUFFADDR        Start write address           GP09286
         LR    R5,R6              Current end of block          GP09286
         SR    R5,R4              Current length                GP09286
*NEXT*   B     WRITBLK            WRITE THE BLOCK               GP09286
         SPACE 1
WRITBLK  AR    R5,R4              Set start and end of write    GP09286
         STM   R4,R5,BUFFADDR     Pass to physical writer       GP09286
         OI    IOPFLAGS,IOFLDATA  SHOW DATA IN BUFFER           GP09286
         FIXWRITE ,               Write physical block          GP09286
         B     WRITEEX            AND RETURN                    GP09286
         SPACE 1
TPUTWRIT CLI   RECFMIX,4          RECFM=V ?                     GP09286
         BE    TPUTWRIV           YES                           GP09286
         SH    R4,=H'4'           BACK UP TO RDW                GP09286
         LA    R5,4(,R5)          LENGTH WITH RDW               GP09286
TPUTWRIV STH   R5,0(,R4)          FILL RDW                      GP09286
         STCM  R5,12,2(R4)          ZERO REST                   GP09286
         L     R6,ZIOECT          RESTORE ECT ADDRESS           GP09286
         L     R7,ZIOUPT          RESTORE UPT ADDRESS           GP09286
         PUTLINE PARM=ZPUTLINE,ECT=(R6),UPT=(R7),ECB=ZIOECB,    GP09286*
               OUTPUT=((R4),DATA),TERMPUT=EDIT,MF=(E,ZIOPL)     GP09286
         SPACE 1
WRITEEX  TM    IOPFLAGS,IOFCURSE  RECURSION REQUESTED?          GP09286
         BNZ   WRITMORE                                         GP09286
         FUNEXIT RC=0                                           GP09286
         SPACE 2
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  ANOTE  - Remember the position in the data set (BSAM/BPAM only)
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
@@ANOTE  FUNHEAD IO=YES,AM=YES,SAVE=SAVEADCB   NOTE position    GP09286
         L     R3,4(,R1)          R3 points to the return value GP09286
         FIXWRITE ,                                             GP09286
         SAM24 ,                  For old code                  GP09286
         TM    IOMFLAGS,IOFEXCP   EXCP mode?                    GP09286
         BZ    NOTEBSAM           No                            GP09286
         L     R4,DCBBLKCT        Return block count            GP09286
         B     NOTECOM                                          GP09286
         SPACE 1
NOTEBSAM NOTE  (R10)              Note current position         GP09286
         LR    R4,R1              Save result                   GP09286
NOTECOM  AMUSE ,                                                GP09286
         ST    R4,0(,R3)          Return TTR0 to user           GP09286
         FUNEXIT RC=0                                           GP09286
         SPACE 2
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  APOINT - Restore the position in the data set (BSAM/BPAM only)
*           Note that this does not fail; it just bombs on the
*           next read or write if incorrect.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
@@APOINT FUNHEAD IO=YES,AM=YES,SAVE=SAVEADCB   NOTE position    GP09286
         L     R3,4(,R1)          R3 points to the TTR value    GP09286
         L     R3,0(,R3)          Get the TTR                   GP09286
         ST    R3,ZWORK           Save below the line           GP09286
         FIXWRITE ,                                             GP09286
         SAM24 ,                  For old code                  GP09286
         TM    IOMFLAGS,IOFEXCP   EXCP mode ?                   GP09286
         BZ    POINBSAM           No                            GP09286
         L     R4,DCBBLKCT        Get current position          GP09286
         SR    R4,R3              Get new position's increment  GP09286
         BZ    POINCOM                                          GP09286
         BM    POINHEAD
POINBACK MVI   TAPECCW,X'27'      Backspace                     GP09286
         B     POINECOM                                         GP09286
POINHEAD MVI   TAPECCW,X'37'      Forward space                 GP09286
POINECOM LA    R0,1                                             GP09286
         STH   R0,TAPECCW+6                                     GP09286
         LPR   R4,R4                                            GP09286
POINELUP EXCP  TAPEIOB                                          GP09286
         WAIT  ECB=TAPEECB                                      GP09286
         BCT   R4,POINELUP                                      GP09286
         ST    R3,DCBBLKCT                                      GP09286
         B     POINCOM                                          GP09286
         SPACE 1
POINBSAM POINT (R10),ZWORK        Request repositioning         GP09286
POINCOM  AMUSE ,                                                GP09286
         NI    IOPFLAGS,255-IOFLEOF   Valid POINT resets EOF    GP09286
         XC    KEPTREC(8),KEPTREC      Also clear record data   GP09286
         FUNEXIT RC=0                                           GP09286
         SPACE 2
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  ACLOSE - Close a data set
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
@@ACLOSE FUNHEAD IO=YES,SAVE=(WORKAREA,WORKLEN,SUBPOOL)  CLOSE  GP09286
         TM    IOMFLAGS,IOFTERM   TERMINAL I/O MODE?            GP09286
         BNZ   FREEBUFF           YES; JUST FREE STUFF          GP09286
         FIXWRITE ,          WRITE FINAL BUFFER, IF ONE         GP09286
FREEBUFF LM    R1,R2,ZBUFF1       Look at first buffer          GP09286
         LTR   R0,R2              Any ?                         GP09286
         BZ    FREEDBF1           No                            GP09286
         FREEMAIN RU,LV=(0),A=(1),SP=SUBPOOL  Free BLOCK buffer GP09286
FREEDBF1 LM    R1,R2,ZBUFF2       Look at first buffer          GP09286
         LTR   R0,R2              Any ?                         GP09286
         BZ    FREEDBF2           No                            GP09286
         FREEMAIN RU,LV=(0),A=(1),SP=SUBPOOL  Free RECRD buffer GP09286
FREEDBF2 TM    IOMFLAGS,IOFTERM   TERMINAL I/O MODE?            GP09286
         BNZ   NOPOOL             YES; SKIP CLOSE/FREEPOOL      GP09286
         CLOSE MF=(E,OPENCLOS)
         TM    DCBBUFCA+L'DCBBUFCA-1,1      BUFFER POOL?        GP09286
         BNZ   NOPOOL             NO, INVALIDATED               GP09286
         SR    R15,R15                                          GP09286
         ICM   R15,7,DCBBUFCA     DID WE GET A BUFFER?          GP09286
         BZ    NOPOOL             0-NO                          GP09286
         FREEPOOL ((R10))
NOPOOL   DS    0H
         FREEMAIN RU,LV=ZDCBLEN,A=(R10),SP=SUBPOOL
         FUNEXIT RC=0                                           GP09286
         SPACE 2                                                GP09286
         PUSH  USING                                            GP09286
         DROP  ,                                                GP09286
*---------------------------------------------------------------------*
*                                                                     *
*  Physical Write - called by @@ACLOSE, switch from output to input   *
*    mode, and whenever output buffer is full or needs to be emptied. *
*  Works for EXCP and BSAM. Special processing for UPDAT mode         *
*---------------------------------------------------------------------*
TRUNCOUT B     *+14-TRUNCOUT(,R15)   SKIP LABEL                 GP09286
         DC    AL1(9),CL(9)'TRUNCOUT' EXPAND LABEL              GP09286
         AIF   ('&SYS' EQ 'S370').NOTRUBS                       GP09286
         DC    0H'0',X'0BE0'      BSM 14,0  PRESERVE AMODE      GP09286
.NOTRUBS STM   R14,R12,12(R13)    SAVE CALLER'S REGISTERS       GP09286
         LR    R12,R15                                          GP09286
         USING TRUNCOUT,R12                                     GP09286
         LA    R15,ZIOSAVE2-ZDCBAREA(,R10)                      GP09286
         ST    R15,8(,R13)                                      GP09286
         ST    R13,4(,R15)                                      GP09286
         LR    R13,R15                                          GP09286
         USING IHADCB,R10    COMMON I/O AREA SET BY CALLER      GP09286
         TM    IOPFLAGS,IOFLDATA   PENDING WRITE ?              GP09286
         BZ    TRUNCOEX      NO; JUST RETURN                    GP09286
         NI    IOPFLAGS,255-IOFLDATA  Reset it                  GP09286
         SAM24 ,             GET LOW                            GP09286
         LM    R4,R5,BUFFADDR  START/NEXT ADDRESS               GP09286
         CLI   RECFMIX,4          RECFM=V?                      GP09286
         BNE   TRUNLEN5                                         GP09286
         SR    R5,R5                                            GP09286
         ICM   R5,3,0(R4)         USE BDW LENGTH                GP09286
         CH    R5,=H'8'           EMPTY ?                       GP09286
         BNH   TRUNPOST           YES; IGNORE REQUEST           GP09286
         B     TRUNTMOD           CHECK OUTPUT TYPE             GP09286
TRUNLEN5 SR    R5,R4              CONVERT TO LENGTH             GP09286
         BNP   TRUNCOEX           NOTHING TO DO                 GP09286
TRUNTMOD DS    0H                                               GP09286
         TM    IOMFLAGS,IOFEXCP   EXCP mode ?                   GP09286
         BNZ   EXCPWRIT           Yes                           GP09286
         CLI   OPENCLOS,X'84'     Update mode?                  GP09286
         BE    TRUNSHRT             Yes; just rewrite as is     GP09286
         CLI   RECFMIX,4          RECFM=F ?                     GP09297
         BNL   *+8                No; leave it alone            GP09297
         STH   R5,DCBBLKSI        Why do I need this?           GP09297
         WRITE DECB,SF,(R10),(R4),(R5),MF=E  Write block        GP09286
         B     TRUNCHK                                          GP09286
TRUNSHRT WRITE DECB,SF,MF=E       Rewrite block from READ       GP09286
TRUNCHK  CHECK DECB                                             GP09286
         B     TRUNPOST           Clean up                      GP09286
         SPACE 1
EXCPWRIT STH   R5,TAPECCW+6                                     GP09286
         STCM  R4,7,TAPECCW+1     WRITE FROM TEXT               GP09286
         NI    DCBIFLGS,255-DCBIFEC   ENABLE ERP                GP09286
         OI    DCBIFLGS,X'40'     SUPPRESS DDR                  GP09286
         STCM  R5,12,IOBSENS0-IOBSTDRD+TAPEIOB   CLEAR SENSE    GP09286
         OI    DCBOFLGS-IHADCB+TAPEDCB,DCBOFLWR  SHOW WRITE     GP09286
         XC    TAPEECB,TAPEECB                                  GP09286
         EXCP  TAPEIOB                                          GP09286
         WAIT  ECB=TAPEECB                                      GP09286
         TM    TAPEECB,X'7F'      GOOD COMPLETION?              GP09286
         BO    TRUNPOST                                         GP09286
*NEXT*   BNO   EXWRN7F            NO                            GP09286
         SPACE 1
EXWRN7F  TM    IOBUSTAT-IOBSTDRD+TAPEIOB,IOBUSB7  END OF TAPE?  GP09286
         BNZ   EXWREND       YES; SWITCH TAPES                  GP09286
         CLC   =X'1020',IOBSENS0-IOBSTDRD+TAPEIOB  EXCEEDED AWS/HET ?
         BNE   EXWRB001                                         GP09286
EXWREND  L     R15,DCBBLKCT                                     GP09286
         SH    R15,=H'1'                                        GP09286
         ST    R15,DCBBLKCT       ALLOW FOR EOF 'RECORD'        GP09286
         EOV   TAPEDCB       TRY TO RECOVER                     GP09286
         B     EXCPWRIT                                         GP09286
         SPACE 1
EXWRB001 LA    R9,TAPEIOB    GET IOB FOR QUICK REFERENCE        GP09286
         ABEND 001,DUMP                                         GP09286
         SPACE 1
TRUNPOST XC    BUFFCURR,BUFFCURR  CLEAR                         GP09286
         CLI   RECFMIX,4          RECFM=V                       GP09286
         BL    TRUNCOEX           F - JUST EXIT                 GP09286
         LA    R4,4               BUILD BDW                     GP09286
         L     R3,BUFFADDR        GET BUFFER                    GP09286
         STH   R4,0(,R3)          UPDATE                        GP09286
TRUNCOEX L     R13,4(,R13)                                      GP09286
         LM    R14,R12,12(R13)    Reload all                    GP09286
         QBSM  0,R14              Return in caller's mode       GP09286
         LTORG ,
         POP   USING ,                                          GP09286
         SPACE 2
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  GETM - GET MEMORY
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
@@GETM   FUNHEAD ,                                              GP09286
*
         LDINT R3,0(,R1)          LOAD REQUESTED STORAGE SIZE
         SLR   R1,R1              PRESET IN CASE OF ERROR       GP09286
         LTR   R4,R3              CHECK REQUEST                 GP09286
         BNP   GETMEX             QUIT IF INVALID               GP09286
*
* To reduce fragmentation, round up size to 64 byte multiple
*
         A     R3,=A(8+(64-1))    OVERHEAD PLUS ROUNDING        GP09286
         N     R3,=X'FFFFFFC0'    MULTIPLE OF 64
*
         GETMAIN RU,LV=(R3),SP=SUBPOOL,&LOCANY                  GP09286
*
* WE STORE THE AMOUNT WE REQUESTED FROM MVS INTO THIS ADDRESS
         ST    R3,0(,R1)
* AND JUST BELOW THE VALUE WE RETURN TO THE CALLER, WE SAVE
* THE AMOUNT THEY REQUESTED
         ST    R4,4(,R1)
         A     R1,=F'8'
*
GETMEX   FUNEXIT RC=(R1)                                        GP09286
         LTORG ,
         SPACE 2
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  FREEM - FREE MEMORY
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
@@FREEM  FUNHEAD ,                                              GP09286
*
         L     R1,0(,R1)                                        GP09286
         S     R1,=F'8'                                         GP09286
         L     R0,0(,R1)                                        GP09286
*
         FREEMAIN RU,LV=(0),A=(1),SP=SUBPOOL                    GP09286
*
         FUNEXIT RC=(15)                                        GP09286
         LTORG ,
         SPACE 2
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  GETCLCK - GET THE VALUE OF THE MVS CLOCK TIMER AND MOVE IT TO AN
*  8-BYTE FIELD.  THIS 8-BYTE FIELD DOES NOT NEED TO BE ALIGNED IN
*  ANY PARTICULAR WAY.
*
*  E.G. CALL 'GETCLCK' USING WS-CLOCK1
*
*  THIS FUNCTION ALSO RETURNS THE NUMBER OF SECONDS SINCE 1970-01-01
*  BY USING SOME EMPIRICALLY-DERIVED MAGIC NUMBERS
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
@@GETCLK FUNHEAD ,                GET TOD CLOCK VALUE           GP09286
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
RETURNGC FUNEXIT RC=(R5)                                        GP09286
         LTORG ,
         SPACE 2
***********************************************************************
**                                                                   **
**   CALL @@SYSTEM,(req-type,pgm-len,pgm-name,parm-len,parm),VL      **
**                                                                   **
**   "-len" fields are self-defining values in the calling list,     **
**       or else pointers to 32-bit signed integer values            **
**                                                                   **
**   "pgm-name" is the address of the name of the program to be      **
**       executed (one to eight characters)                          **
**                                                                   **
**   "parm" is the address of a text string of length "parm-len",    **
**       and may be zero to one hundred bytes (OS JCL limit)         **
**                                                                   **
**   "req-type" is or points to 1 for a program ATTACH               **
**                              2 for TSO CP invocation              **
**                                                                   **
*---------------------------------------------------------------------*
**                                                                   **
**    Author:  Gerhard Postpischil                                   **
**                                                                   **
**    This program is placed in the public domain.                   **
**                                                                   **
*---------------------------------------------------------------------*
**                                                                   **
**    Assembly: Any MVS or later assembler may be used.              **
**       Requires SYS1.MACLIB. TSO CP support requires additional    **
**       macros from SYS1.MODGEN (SYS1.AMODGEN in MVS).              **
**       Intended to work in any 24 and 31-bit environment.          **
**                                                                   **
**    Linker/Binder: RENT,REFR,REUS                                  **
**                                                                   **
*---------------------------------------------------------------------*
**    Return codes:  when R15:0 R15 has return from program.         **
**    Else R15 is 0480400n   GETMAIN failed                          **
**      R15 is 04806nnn  ATTACH failed                               **
**      R15 is 1400000n  PARM list error: n= 1,2, or 3 (req/pgm/parm)**
***********************************************************************
@@SYSTEM FUNHEAD ,                ISSUE OS OR TSO COMMAND       GP09286
         L     R15,4(,R13)        OLD SA                        GP09286
         LA    R11,16(,R15)       REMEMBER THE RETURN CODE ADDRESS
         MVC   0(4,R11),=X'04804000'  PRESET FOR GETMAIN FAILURE
         LR    R9,R1              SAVE PARAMETER LIST ADDRESS
         LA    R0,SYSATDLN        GET LENGTH OF SAVE AND WORK AREA
         GETMAIN RC,LV=(0)        GET STORAGE
         LTR   R15,R15            SUCCESSFUL ?
         BZ    SYSATHAV           YES
         STC   R15,3(,R11)        SET RETURN VALUES
         B     SYSATRET           RELOAD AND RETURN
*    CLEAR GOTTEN STORAGE AND ESTABLISH SAVE AREA
*
SYSATHAV ST    R1,8(,R13)         LINK OURS TO CALLER'S SAVE AREA
         ST    R13,4(,R1)         LINK CALLER'S TO OUR AREA
         LR    R13,R1
         USING SYSATWRK,R13
         XC    SYSATZER,SYSATZER  CLEAR DYNAMIC STUFF
         MVC   0(4,R11),=X'14000002'  PRESET FOR PARM ERROR
         LDINT R4,0(,R9)          REQUEST TYPE
         LDINT R5,4(,R9)          LENGTH OF PROGRAM NAME
         L     R6,8(,R9)          -> PROGRAM NAME
         LDINT R7,12(,R9)         LENGTH OF PARM
         L     R8,16(,R9)         -> PARM TEXT
*   NOTE THAT THE CALLER IS EITHER COMPILER CODE, OR A COMPILER
*   LIBRARY ROUTINE, SO WE DO MINIMAL VALIDITY CHECKING
*
*   EXAMINE PROGRAM NAME LENGTH AND STRING
*
         LTR   R5,R5              ANY LENGTH ?
         BNP   SYSATEXT           NO; OOPS
         CH    R5,=H'8'           NOT TOO LONG ?
         BH    SYSATEXT           TOO LONG; TOO BAD
         BCTR  R5,0
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
         L     R1,12(,R1)                                       GP09101
         LA    R1,0(,R1)     CLEAR EOL BIT IN EITHER AMODE      GP09101
         LTR   R1,R1         ANY ADDRESS?                       GP09101
         BZ    SYSATCOM      NO; SKIP                           GP09101
         PUSH  USING         (FOR LATER ADDITIONS?)             GP09101
         USING ECT,R1        DECLARE ECT                        GP09101
         LM    R14,R15,SYSATPGM   GET COMMAND NAME              GP09101
         LA    R0,7          MAX TEST/SHIFT                     GP09101
SYSATLCM CLM   R14,8,=CL11' '  LEADING BLANK ?                  GP09101
         BNE   SYSATLSV      NO; SET COMMAND NAME               GP09101
         SLDL  R14,8         ELIMINATE LEADING BLANK            GP09101
         IC    R15,=CL11' '  REPLACE BY TRAILING BLANK          GP09101
         BCT   R0,SYSATLCM   TRY AGAIN                          GP09101
SYSATLSV STM   R14,R15,ECTPCMD                                  GP09101
         NI    ECTSWS,255-ECTNOPD      SET FOR OPERANDS EXIST   GP09101
         EX    R7,SYSAXBLK   SEE IF ANY OPERANDS                GP09101
         BNE   SYSATCOM           HAVE SOMETHING                GP09101
         OI    ECTSWS,ECTNOPD     ALL BLANK                     GP09101
SYSATCOM LA    R1,SYSATPRM        PASS ADDRESS OF PARM ADDRESS
         LA    R2,SYSATPGM        POINT TO NAME
         LA    R3,SYSATECB        AND ECB
         ATTACH EPLOC=(R2),       INVOKE THE REQUESTED PROGRAM         *
               ECB=(R3),SF=(E,SYSATLST)
         LTR   R0,R15             CHECK RETURN CODE
         BZ    SYSATWET           GOOD
         MVC   0(4,R11),=X'04806000'  ATTACH FAILED
         STC   R15,3(,R11)        SET ERROR CODE
         B     SYSATEXT           FAIL
SYSATWET ST    R1,SYSATTCB        SAVE FOR DETACH
         WAIT  ECB=SYSATECB       WAIT FOR IT TO FINISH
         MVC   1(3,R11),SYSATECB+1
         MVI   0(R11),0           SHOW CODE FROM PROGRAM
         DETACH SYSATTCB          GET RID OF SUBTASK
         B     SYSATEXT           AND RETURN
SYSAXPGM OC    SYSATPGM(0),0(R6)  MOVE NAME AND UPPER CASE
SYSAXTXT MVC   SYSATOTX(0),0(R8)    MOVE PARM TEXT
SYSAXBLK CLC   SYSATOTX(0),SYSATOTX-1  TEST FOR OPERANDS        GP09101
*    PROGRAM EXIT, WITH APPROPRIATE RETURN CODES
*
SYSATEXT LR    R1,R13        COPY STORAGE ADDRESS
         L     R9,4(,R13)    GET CALLER'S SAVE AREA
         LA    R0,SYSATDLN   GET ORIGINAL LENGTH
         FREEMAIN R,A=(1),LV=(0)  AND RELEASE THE STORAGE
         L     R13,4(,R13)   RESTORE CALLER'S SAVE AREA
SYSATRET FUNEXIT ,           RESTORE REGS; SET RETURN CODES     GP09286
         SPACE 1             RETURN TO CALLER
*    DYNAMICALLY ACQUIRED STORAGE
*
SYSATWRK DSECT ,             MAP STORAGE
         DS    18A           OUR OS SAVE AREA
SYSATCLR DS    0F            START OF CLEARED AREA
SYSATECB DS    F             EVENT CONTROL FOR SUBTASK
SYSATTCB DS    A             ATTACH TOKEN FOR CLEAN-UP
SYSATPRM DS    4A            PREFIX FOR CP
SYSATOPL DS    2Y     1/4    PARM LENGTH / LENGTH SCANNED
SYSATPGM DS    CL8    2/4    PROGRAM NAME (SEPARATOR)
SYSATOTL DS    Y      3/4    OS PARM LENGTH / BLANKS FOR CP CALL
SYSATOTX DS    CL100  4/4    NORMAL PARM TEXT STRING
SYSATLST ATTACH EPLOC=SYSATPGM,ECB=SYSATECB,SF=L
SYSATZER EQU   SYSATCLR,*-SYSATCLR,C'X'   ADDRESS & SIZE TO CLEAR
SYSATDLN EQU   *-SYSATWRK     LENGTH OF DYNAMIC STORAGE
MVSSUPA  CSECT ,             RESTORE
         SPACE 2
***********************************************************************
**                                                                   **
**   INVOKE IDCAMS: CALL @@IDCAMS,(@LEN,@TEXT)                       **
**                                                                   **
***********************************************************************
         PUSH  USING
         DROP  ,
@@IDCAMS FUNHEAD ,                EXECUTE IDCAMS REQUEST        GP09286
         LA    R1,0(,R1)          ADDRESS OF IDCAMS REQUEST (V-CON)
         ST    R1,IDC@REQ         SAVE REQUEST ADDRESS
         MVI   EXFLAGS,0          INITIALIZE FLAGS
         LA    R1,AMSPARM         PASS PARAMETER LIST
         LINK  EP=IDCAMS          INVOKE UTILITY
         FUNEXIT RC=(15)          RESTORE CALLER'S REGS         GP09286
         POP   USING
         SPACE 1
***************************************************************
* IDCAMS ASYNCHRONOUS EXIT ROUTINE
***************************************************************
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
XIDCGET  TM    EXFLAGS,EXFGET            X'FF' = PRIOR GET ISSUED ?
         BNZ   XIDCGET4                  YES, SET RET CODE = 04
         L     R1,IDC@REQ         GET REQUEST ADDRESS
         LDINT R3,0(,R1)          LOAD LENGTH
         L     R2,4(,R1)          LOAD TEXT POINTER
         LA    R2,0(,R2)          CLEAR HIGH
         STM   R2,R3,0(R5)        PLACE INTO IDCAMS LIST
         OI    EXFLAGS,EXFGET            X'FF' = A GET HAS BEEN ISSUED
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
XIDCSHOW DS    0H
*DEBUG*  WTO   MF=(E,AMSPRINT)    SHOW MESSAGE
XIDCPUTZ SR    R15,R15
         B     XIDCEXIT
XIDCSKIP OI    EXFLAGS,EXFSKIP    SKIP THIS AND REMAINING MESSAGES
         SR    R15,R15
***************************************************************
* IDCAMS ASYNC EXIT ROUTINE - EXIT, CONSTANTS & WORKAREAS
***************************************************************
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
**                                                                   **
**   CALL @@DYNAL,(ddn-len,ddn-adr,dsn-len,dsn-adr),VL               **
**                                                                   **
**   "-len" fields are self-defining values in the calling list,     **
**       or else pointers to 32-bit signed integer values            **
**                                                                   **
**   "ddn-adr"  is the address of the DD name to be used. When the   **
**       contents is hex zero or blank, and len=8, gets assigned.    **
**                                                                   **
**   "dsn-adr" is the address of a 1 to 44 byte data set name of an  **
**       existing file (sequential or partitioned).                  **
**                                                                   **
**   Calling @@DYNAL with a DDNAME and a zero length for the DSN     **
**   results in unallocation of that DD (and a PARM error).          **
**                                                                   **
*---------------------------------------------------------------------*
**                                                                   **
**    Author:  Gerhard Postpischil                                   **
**                                                                   **
**    This program is placed in the public domain.                   **
**                                                                   **
*---------------------------------------------------------------------*
**                                                                   **
**    Assembly: Any MVS or later assembler may be used.              **
**       Requires SYS1.MACLIB                                        **
**       Intended to work in any 24 and 31-bit environment.          **
**                                                                   **
**    Linker/Binder: RENT,REFR,REUS                                  **
**                                                                   **
*---------------------------------------------------------------------*
**    Return codes:  R15:04sssnnn   it's a program error code:       **
**    04804 - GETMAIN failed;  1400000n   PARM list error            **
**                                                                   **
**    Otherwise R15:0-1  the primary allocation return code, and     **
**      R15:2-3 the reason codes.                                    **
***********************************************************************
*  Maintenance:                                     new on 2008-06-07 *
*                                                                     *
***********************************************************************
         ENTRY @@DYNAL
@@DYNAL  STM   R14,R12,12(R13)    SAVE CALLER'S REGISTERS
         LR    R12,R15            ESTABLISH MY BASE
         USING @@DYNAL,R12        AND DECLARE IT
         LA    R11,16(,R13)       REMEMBER THE RETURN CODE ADDRESS
         MVC   0(4,R11),=X'04804000'  PRESET
         LR    R9,R1              SAVE PARAMETER LIST ADDRESS
         LA    R0,DYNALDLN        GET LENGTH OF SAVE AND WORK AREA
         GETMAIN RC,LV=(0)        GET STORAGE
         LTR   R15,R15            SUCCESSFUL ?
         BZ    DYNALHAV           YES
         STC   R15,3(,R11)        SET RETURN VALUES
         B     DYNALRET           RELOAD AND RETURN
         SPACE 1
*    CLEAR GOTTEN STORAGE AND ESTABLISH SAVE AREA
*
DYNALHAV ST    R1,8(,R13)         LINK OURS TO CALLER'S SAVE AREA
         ST    R13,4(,R1)         LINK CALLER'S TO OUR AREA
         LR    R13,R1
         USING DYNALWRK,R13
         MVC   0(4,R11),=X'14000001'  PRESET FOR PARM LIST ERROR
         MVC   DYNLIST(ALLDYNLN),PATLIST  INITIALIZE EVERYTHING
         LDINT R4,0(,R9)          DD NAME LENGTH
         L     R5,4(,R9)          -> DD NAME
         LDINT R6,8(,R9)          DSN LENGTH
         L     R7,12(,R9)         -> DATA SET NAME
         SPACE 1
*   NOTE THAT THE CALLER IS EITHER COMPILER CODE, OR A COMPILER
*   LIBRARY ROUTINE, SO WE DO MINIMAL VALIDITY CHECKING
*
         SPACE 1
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
         SPACE 1
*   COMPLETE REQUEST WITH CALLER'S DATA
*
         LTR   R4,R4              CHECK DDN LENGTH
         BNP   DYNALEXT           OOPS
         CH    R4,=AL2(L'ALLADDN)   REASONABLE SIZE ?
         BH    DYNALEXT           NO
         BCTR  R4,0
         EX    R4,DYNAXDDN        MOVE DD NAME
         OC    ALLADDN,=CL11' '   CONVERT HEX ZEROES TO BLANKS  GP09101
         CLC   ALLADDN,=CL11' '   NAME SUPPLIED ?               GP09101
         BNE   DYNALDDN           YES
         MVI   ALLAXDDN+1,DALRTDDN  REQUEST RETURN OF DD NAME
         CH    R4,=AL2(L'ALLADDN-1)   CORRECT SIZE FOR RETURN ?
         BE    DYNALNDD           AND LEAVE R5 NON-ZERO
         B     DYNALEXT           NO
         SPACE 1
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
         SPACE 1
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
         SPACE 1
*    PROGRAM EXIT, WITH APPROPRIATE RETURN CODES
*
DYNALEXT LR    R1,R13        COPY STORAGE ADDRESS
         L     R9,4(,R13)    GET CALLER'S SAVE AREA
         LA    R0,DYNALDLN   GET ORIGINAL LENGTH
         FREEMAIN R,A=(1),LV=(0)  AND RELEASE THE STORAGE
         L     R13,4(,R13)   RESTORE CALLER'S SAVE AREA
DYNALRET LM    R14,R12,12(R13) RESTORE REGISTERS; SET RETURN CODES
         BR    R14           RETURN TO CALLER
         SPACE 2
         LTORG ,
         PUSH  PRINT
         PRINT NOGEN         DON'T NEED TWO COPIES
PATLIST  DYNPAT P=PAT        EXPAND ALLOCATION DATA
         POP   PRINT
         SPACE 2
*    DYNAMICALLY ACQUIRED STORAGE
*
DYNALWRK DSECT ,             MAP STORAGE
         DS    18A           OUR OS SAVE AREA
         SPACE 1
DYNLIST  DYNPAT P=ALL        EXPAND ALLOCATION DATA
DYNALDLN EQU   *-DYNALWRK     LENGTH OF DYNAMIC STORAGE
         CSECT ,             RESTORE
*
*
*
* Keep this code last because it uses different base register
*
         PUSH  USING                                            GP09286
         DROP  ,                                                GP09286
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  SETJ - SAVE REGISTERS INTO ENV
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
         ENTRY @@SETJ
         USING @@SETJ,R15
@@SETJ   L     R15,0(,R1)         get the env variable
         STM   R0,R14,0(R15)      save registers to be restored
         LA    R15,0              setjmp needs to return 0
         BR    R14                return to caller
         POP   USING                                            GP09286
         SPACE 1
         PUSH  USING                                            GP09286
         DROP  ,                                                GP09286
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  LONGJ - RESTORE REGISTERS FROM ENV
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
         ENTRY @@LONGJ
         USING @@LONGJ,R15
@@LONGJ  L     R2,0(,R1)          get the env variable
         L     R15,60(,R2)        get the return code
         LM    R0,R14,0(R2)       restore registers
         BR    R14                return to caller
         POP   USING                                            GP09286
         SPACE 2
* S/370 doesn't support switching modes so this code is useless,
* and won't compile anyway because "BSM" is not known.
*
         AIF   ('&SYS' EQ 'S370').NOMODE  If S/370 we can't switch mode
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  SETM24 - Set AMODE to 24
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
         ENTRY @@SETM24
         USING @@SETM24,R15
@@SETM24 LA    R14,0(,R14)        Sure hope caller is below the line
         BSM   0,R14              Return in amode 24
         SPACE 1
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  SETM31 - Set AMODE to 31
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
         ENTRY @@SETM31
         USING @@SETM31,R15
@@SETM31 O     R14,=X'80000000'   Set to switch mode            GP09286
         ORG   *-4      *****TEMPORAY UNTIL BAL FIXED TO BAS    GP09286
         ICM   R14,8,=X'80'       Clobber R14                   GP09286
         BSM   0,R14              Return in amode 31
         LTORG ,
*
.NOMODE  ANOP  ,                  S/370 doesn't support MODE switching
         SPACE 2
*EXTRA*  IEZIOB                   Input/Output Block
         SPACE 2
WORKAREA DSECT
SAVEAREA DS    18F
DWORK    DS    D                  Extra work space              GP09286
WWORK    DS    D                  Extra work space              GP09286
DWDDNAM  DS    D                  Extra work space              GP09296
WORKLEN  EQU   *-WORKAREA
JFCB     DS    0F
         IEFJFCBN LIST=YES        Job File Control Block
CAMLST   DS    XL(CAMLEN)         CAMLST for OBTAIN to get VTOC entry
* Format 1 Data Set Control Block
*   N.B. Current program logic does not use DS1DSNAM, leaving 44 bytes
*     of available space
         IECSDSL1 1               Map the Format 1 DSCB
DSCBCCHH DS    CL5                CCHHR of DSCB returned by OBTAIN
         DS    CL47               Rest of OBTAIN's 148 byte work area
OPENLEN  EQU   *-WORKAREA         Length for @@AOPEN processing GP09286
         SPACE 2
***********************************************************************
*                                                                     *
*   FILE ACCESS CONTROL BLOCK (N.B.-STARTS WITH DCBD DUE TO DSECT)    *
*   CONTAINS DCB, DECB, JFCB, DSCB 1, BUFFER POINTERS, FLAGS, ETC.    *
*                                                                     *
***********************************************************************
         DCBD  DSORG=PS,DEVD=(DA,TA)   Map Data Control Block   GP09286
         ORG   IHADCB             Overlay the DCB DSECT
ZDCBAREA DS    0H
         DS    CL(BSAMDCBN)                                     GP09286
         READ  DECB,SF,IHADCB,2-2,3-3,MF=L  READ/WRITE BSAM     GP09286
         ORG   IHADCB             Only using one DCB
         DS    CL(QSAMDCBL)         so overlay this one
         ORG   IHADCB             Only using one DCB            GP09286
TAPEDCB  DCB   DDNAME=TAPE,MACRF=E,DSORG=PS,REPOS=Y,BLKSIZE=0,         *
               DEVD=TA                 LARGE SIZE               GP09286
         ORG   TAPEDCB+84    LEAVE ROOM FOR DCBLRECL            GP09286
ZXCPVOLS DC    F'0'          VOLUME COUNT                       GP09286
TAPECCW  CCW   1,3-3,X'40',4-4                                  GP09286
         CCW   3,3-3,X'20',1                                    GP09286
TAPEXLEN EQU   *-TAPEDCB     PATTERN TO MOVE                    GP09286
TAPEECB  DC    A(0)                                             GP09286
TAPEIOB  DC    X'42,00,00,00'                                   GP09286
         DC    A(TAPEECB)                                       GP09286
         DC    2A(0)                                            GP09286
         DC    A(TAPECCW)                                       GP09286
         DC    A(TAPEDCB)                                       GP09286
         DC    2A(0)                                            GP09286
         SPACE 1
         ORG   IHADCB
ZPUTLINE PUTLINE MF=L        PATTERN FOR TERMINAL I/O           GP09286
*DSECT*  IKJIOPL ,                                              GP09286
         SPACE 1                                                GP09286
ZIOPL    DS    0A            MANUAL EXPANSION TO AVOID DSECT    GP09286
IOPLUPT  DS    A        PTR TO UPT                              GP09286
IOPLECT  DS    A        PTR TO ECT                              GP09286
IOPLECB  DS    A        PTR TO USER'S ECB                       GP09286
IOPLIOPB DS    A        PTR TO THE I/O SERVICE RTN PARM BLOCK   GP09286
ZIOECB   DS    A                   TPUT ECB                     GP09286
ZIOECT   DS    A                   ORIGINATING ECT              GP09286
ZIOUPT   DS    A                   UPT                          GP09286
ZIODDNM  DS    CL8      DD NAME AT OFFSET X'28' FOR DCB COMPAT. GP09286
ZGETLINE GETLINE MF=L             TWO WORD GTPB                 GP09286
         ORG   ,
OPENCLOS DS    A                  OPEN/CLOSE parameter list
DCBXLST  DS    2A                 07 JFCB / 85 DCB EXIT         GP09286
EOFR24   DS    CL(EOFRLEN)
ZBUFF1   DS    A,F                Address, length of buffer     GP09286
ZBUFF2   DS    A,F                Address, length of 2nd buffer GP09286
KEPTREC  DS    A,F                Address & length of saved rcd GP09286
*
BLKSIZE  DS    F                  Save area for input DCB BLKSIZE
LRECL    DS    F                  Save area for input DCB LRECL
BUFFADDR DS    A     1/3          Location of the BLOCK Buffer
BUFFCURR DS    A     2/3          Current record in the buffer
BUFFEND  DS    A     3/3          Address after end of current block
VBSADDR  DS    A                  Location of the VBS record build area
VBSEND   DS    A                  Addr. after end VBS record build area
         SPACE 1
ZWORK    DS    D             Below the line work storage        GP09286
DEVINFO  DS    2F                 UCB Type / Max block size
MEMBER24 DS    CL8
RECFMIX  DS    X             Record format index: 0-F 4-V 8-U
IOMFLAGS DS    X             Remember open MODE                 GP09286
IOFOUT   EQU   X'01'           Output mode                      GP09286
IOFEXCP  EQU   X'08'           Use EXCP for TAPE                GP09286
IOFBLOCK EQU   X'10'           Using BSAM READ/WRITE mode       GP09286
IOFTERM  EQU   X'80'           Using GETLINE/PUTLINE            GP09286
IOPFLAGS DS    X             Remember prior events
IOFKEPT  EQU   X'01'           Record info kept                 GP09286
IOFCONCT EQU   X'02'           Reread - unlike concatenation    GP09286
IOFDCBEX EQU   X'04'           DCB exit entered                 GP09286
IOFCURSE EQU   X'08'           Write buffer recursion           GP09286
IOFLIOWR EQU   X'10'           Last I/O was Write type          GP09286
IOFLDATA EQU   X'20'           Output buffer has data           GP09286
IOFLSDW  EQU   X'40'           Spanned record incomplete
IOFLEOF  EQU   X'80'           Encountered an End-of-File
FILEMODE DS    X             AOPEN requested record format dftl GP09286
FMFIX    EQU   0               Fixed RECFM (blocked)            GP09286
FMVAR    EQU   1               Variable (blocked)               GP09286
FMUND    EQU   2               Undefined                        GP09286
ZRECFM   DS    X             Equivalent RECFM bits              GP09286
ZIOSAVE2 DS    18F           Save area for physical write       GP09286
SAVEADCB DS    18F                Register save area for PUT
ZDCBLEN  EQU   *-ZDCBAREA
         SPACE 2
         PRINT NOGEN
         IHAPSA ,            MAP LOW STORAGE
         IKJTCB ,            MAP TASK CONTROL BLOCK
         IKJECT ,            MAP ENV. CONTROL BLOCK             GP09101
         IKJPTPB ,           PUTLINE PARAMETER BLOCK            GP09286
         IKJCPPL ,                                              GP09286
         IKJPSCB ,                                              GP09286
         IEZJSCB ,                                              GP09286
         IEZIOB ,                                               GP09286
         IEFZB4D0 ,          MAP SVC 99 PARAMETER LIST
         IEFZB4D2 ,          MAP SVC 99 PARAMETERS
         IEFUCBOB ,                                             GP09286
         END   ,
