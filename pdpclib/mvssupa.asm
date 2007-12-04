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
*  still there, it's just being tested after any change.
*
***********************************************************************
*
* Note that the VBS support may not be properly implemented.
* Note that this code issues WTOs. It should be changed to just
* set a return code an exit gracefully instead. I'm not talking
* about that dummy WTO. But on the subject of that dummy WTO - it
* should be made consistent with the rest of PDPCLIB which doesn't
* use that to set the RMODE/AMODE. It should be consistent one way
* or the other.
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
*
         LCLC &COMP               Declare compiler switch
&COMP    SETC 'GCC'               Indicate that this is for GCC
* &COMP    SETC 'C370'            Indicate that this is for C/370
         LCLC &SYS                Declare variable for system
&SYS     SETC 'S380'              Indicate that this is for S/380
*
         CSECT
         PRINT NOGEN
* YREGS IS NOT AVAILABLE WITH IFOX
*         YREGS
SUBPOOL  EQU   0
R0       EQU   0
R1       EQU   1
R2       EQU   2
R3       EQU   3
R4       EQU   4
R5       EQU   5
R6       EQU   6
R7       EQU   7
R8       EQU   8
R9       EQU   9
R10      EQU   10
R11      EQU   11
R12      EQU   12
R13      EQU   13
R14      EQU   14
R15      EQU   15
         ENTRY @@AOPEN
@@AOPEN  EQU   *
         SAVE  (14,12),,@@AOPEN  Save caller's regs.
         LR    R12,R15
         USING @@AOPEN,R12
         LR    R11,R1
*
         GETMAIN RU,LV=WORKLEN,SP=SUBPOOL
         ST    R13,4(,R1)
         ST    R1,8(,R13)
         LR    R13,R1
         LR    R1,R11
         USING WORKAREA,R13
*
         L     R3,00(,R1)         R3 POINTS TO DDNAME
         L     R4,04(,R1)         R4 is the MODE.  0=input 1=output
         L     R5,08(,R1)         R5 POINTS TO RECFM
         L     R8,12(,R1)         R8 POINTS TO LRECL
         L     R9,16(,R1)         R9 POINTS TO MEMBER NAME (OF PDS)
         LA    R9,00(,R9)         Strip off high-order bit or byte
*
* Because of the AMODE, RMODE, and  LOC=BELOW, we need to know
* whether assembling for MVS 3.8j or a 31 bit or higher MVS.
* The MACLIB is different for the systems so MACRO WTO will
* be used to differentiate between MACLIBs.
         GBLA  &IHBLEN            Declare WTO Global length
DUMMYWTO WTO   ' '                Generate WTO to see if Global set
         ORG   DUMMYWTO           Overlay dummy WTO
         LCLA  &MVS38J            Declare Local for MACLIB level
&MVS38J  SETA  &IHBLEN            Zero if MVS 3.8j MACLIBs
         AIF   (&MVS38J NE 0).MVS3164  If not MVS 3.8j, then 31/64 bit
         GETMAIN RU,LV=ZDCBLEN,SP=SUBPOOL  No LOC= for MVS 3.8j GETMAIN
         AGO   .GETOEND
.MVS3164 ANOP  ,                  31 or 64 bit MVS
         GETMAIN RU,LV=ZDCBLEN,SP=SUBPOOL,LOC=BELOW
         AMODE ANY                Functions called from either AMODE
         RMODE ANY                Program resides above or below line
.GETOEND ANOP
*
         LR    R2,R1              Addr.of storage obtained to its base
         USING IHADCB,R2          Give assembler DCB area base register
         LR    R0,R2              Load output DCB area address
         LA    R1,ZDCBLEN         Load output length of DCB area
         LA    R10,0              No input location
         LA    R11,0              Pad of X'00' and no input length
         MVCL  R0,R10             Clear DCB area to binary zeroes
         AIF   ('&COMP' NE 'C370').GCCMODE
         L     R4,0(,R4)          Load C/370 MODE.  0=input 1=output
.GCCMODE ANOP
         LTR   R4,R4              See if OPEN input or output
         BNZ   WRITING
* READING
         MVC   ZDCBAREA(INDCBLN),INDCB  Move input DCB template to work
         LTR   R9,R9              See in an address for the member name
         BZ    NEXTMVC            No member, leave DCB DSORG=PS
         MVI   DCBDSRG1,DCBDSGPO  Replace DCB DSORG=PS with PO
NEXTMVC  DS    0H
         MVC   EOFR24(EOFRLEN),ENDFILE
         MVC   DECB(READLEN),READDUM  MF=L READ MACRO to work area
         LA    R10,JFCB
         ST    R10,JFCBPTR
         MVI   JFCBPTR,X'87'  Exit type 7 + 80 (End of list indicator)
         LA    R10,JFCBPTR
         LA    R1,EOFR24
         STCM  R1,B'0111',DCBEODA
         STCM  R10,B'0111',DCBEXLSA
         MVC   DCBDDNAM,0(R3)
         MVI   OPENCLOS,X'80'     Initialize MODE=24 OPEN/CLOSE list
         LTR   R9,R9              See if an address for the member name
         BNZ   OPENIN             Is member name, skip changing DCB
         RDJFCB ((R2)),MF=(E,OPENCLOS)  Read JOB File Control Block
         TM    JFCBIND1,JFCPDS    See if a member name in JCL
         BO    OPENIN             Is member name, skip changing DCB
         MVC   CAMLST,CAMDUM      Copy CAMLST template to work area
         LA    R1,JFCB            Load address of input data set name
         ST    R1,CAMLST+4        Store data set name address in CAMLST
         LA    R1,JFCBVOLS        Load address of input data set volser
         ST    R1,CAMLST+8        Store data set name volser in CAMLST
         LA    R1,DSCB+44         Load address of where to put DSCB
         ST    R1,CAMLST+12       Store CAMLST output loc. in CAMLST
         OBTAIN CAMLST            Read the VTOC record
* CAMLST CAMLST SEARCH,DSNAME,VOLSER,DSCB+44
* See if DSORG=PO but no member so set LRECL&BLKSIZE=256 read directory
         TM    DS1DSORG,DS1DSGPO  See if DSORG=PO
         BZ    OPENIN             Not PDS, don't read PDS directory
* At this point, we have a PDS but no member name requested.
* Request must be to read the PDS directory
         MVI   DCBBLKSI,1         Set DCB BLKSIZE to 256
         MVI   DCBLRECL,1         Set DCB LRECL to 256
         MVI   DCBRECFM,DCBRECF   Set DCB RECFM to RECFM=F
OPENIN   DS    0H
         OPEN  ((R2),INPUT),MF=(E,OPENCLOS)
         TM    DCBOFLGS,DCBOFOPN  Did OPEN work?
         BZ    BADOPIN            OPEN failed, go return error code -37
         MVC   LRECL+2(2),DCBLRECL  Copy LRECL to a fullword
         MVC   BLKSIZE+2(2),DCBBLKSI  Copy BLKSIZE to a fullword
         LTR   R9,R9              See if an address for the member name
         BZ    GETBUFF            No member name, skip finding it
*
         AIF   ('&SYS' NE 'S380').N380OP1
* Get R9 below the line
         MVC   JFCBELNM,0(R9)
         LA    R9,JFCBELNM
.N380OP1 ANOP
*
         FIND  (R2),(R9),D        Point to the requested member
*
         LTR   R15,R15            See if member found
         BZ    GETBUFF            Member found, go get an input buffer
* If FIND return code not zero, process return and reason codes and
* return to caller with a negative return code.
         SLL   R15,8              Shift return code for reason code
         OR    R15,R0             Combine return code and reason code
         LR    R3,R15             Number to generate return and reason
         MVI   OPENCLOS,X'80'     Initialize MODE=24 OPEN/CLOSE list
         CLOSE ((R2)),MF=(E,OPENCLOS)  Close, FREEPOOL not needed
FREEDCB  DS    0H
         FREEMAIN RU,LV=ZDCBLEN,A=(2),SP=SUBPOOL  Free DCB area
         LCR   R2,R3              Set return and reason code
         B     RETURNOP           Go return to caller with negative RC
BADOPIN  DS    0H
         LA    R3,37              Load OPEN input error return code
         B     FREEDCB            Go free the DCB area
BADOPOUT DS    0H
         LA    R3,39              Load OPEN output error return code
         B     FREEDCB            Go free the DCB area
GETBUFF  DS    0H
         L     R6,BLKSIZE         Load the input blocksize
         LA    R6,4(,R6)          Add 4 in case RECFM=U buffer
         GETMAIN RU,LV=(R6),SP=SUBPOOL  Get input buffer storage
         ST    R1,BUFFADDR        Save the buffer address for READ
         XC    0(4,R1),0(R1)      Clear the RECFM=U Record Desc. Word
         TM    DCBRECFM,DCBRECV+DCBRECSB  See if spanned records
         BNO   DONEOPEN           Not RECFM=VS, VBS, etc. spanned, go
         L     R6,LRECL           Load the input VBS LRECL
         CL    R6,F32760          See if LRECL=X
         BNH   GETVBS             Not LRECL=X, just get LRECL
         L     R6,F65536          Allow records up to 64K input
GETVBS   DS    0H
         GETMAIN RU,LV=(R6),SP=SUBPOOL  Get VBS build record area
         ST    R1,VBSADDR         Save the VBS record build area addr.
         AR    R1,R6              Add size GETMAINed to find end
         ST    R1,VBSEND          Save address after VBS rec.build area
*        XC    VBSCURR,VBSCURR    VBS current record location is zero
         B     DONEOPEN           Go return to caller with DCB info
*
WRITING  DS    0H
         MVC   ZDCBAREA(OUTDCBLN),OUTDCB
         LA    R10,JFCB
* EXIT TYPE 07 + 80 (END OF LIST INDICATOR)
         ICM   R10,B'1000',=X'87'
         ST    R10,JFCBPTR
         LA    R10,JFCBPTR
         STCM  R10,B'0111',DCBEXLSA
         MVC   DCBDDNAM,0(R3)
         MVI   OPENCLOS,X'80'     Initialize MODE=24 OPEN/CLOSE list
         RDJFCB ((R2)),MF=(E,OPENCLOS)  Read JOB File Control Blk
         LTR   R9,R9
         BZ    WNOMEM
         MVC   JFCBELNM,0(R9)
         OI    JFCBIND1,JFCPDS
         OPEN  ((R2),OUTPUT),MF=(E,OPENCLOS),TYPE=J
         B     WOPENEND           Go to move DCB info
WNOMEM   DS    0H
         TM    JFCBIND1,JFCPDS    See if a member name in JCL
         BO    WNOMEM2            Is member name, go to continue OPEN
         MVC   CAMLST,CAMDUM      Copy CAMLST template to work area
         LA    R1,JFCB            Load address of output data set name
         ST    R1,CAMLST+4        Store data set name address in CAMLST
         LA    R1,JFCBVOLS        Load addr. of output data set volser
         ST    R1,CAMLST+8        Store data set name volser in CAMLST
         LA    R1,DSCB+44         Load address of where to put DSCB
         ST    R1,CAMLST+12       Store CAMLST output loc. in CAMLST
         OBTAIN CAMLST            Read the VTOC record
* CAMLST CAMLST SEARCH,DSNAME,VOLSER,DSCB+44
* See if DSORG=PO but no member so OPEN output would destroy directory
         TM    DS1DSORG,DS1DSGPO  See if DSORG=PO
         BZ    WNOMEM2            Is not PDS, go OPEN
         WTO   'MVSSUPA - No member name for output PDS',ROUTCDE=11
         WTO   'MVSSUPA - Refuses to write over PDS directory',        C
               ROUTCDE=11
         ABEND 123                Abend without a dump
         DC    H'0'               Insure that abend took
WNOMEM2  DS    0H
         OPEN  ((R2),OUTPUT),MF=(E,OPENCLOS)
WOPENEND DS    0H
         TM    DCBOFLGS,DCBOFOPN  Did OPEN work?
         BZ    BADOPOUT           OPEN failed, go return error code -39
         MVC   LRECL+2(2),DCBLRECL  Copy LRECL to a fullword
         MVC   BLKSIZE+2(2),DCBBLKSI  Copy BLKSIZE to a fullword
DONEOPEN DS    0H
         L     R1,LRECL           Load RECFM F or V max. record length
         TM    DCBRECFM,DCBRECU   See if RECFM=U
         BNO   NOTU               Not RECFM=U, go leave LRECL as LRECL
         L     R1,BLKSIZE         Load RECFM U maximum record length
         LTR   R4,R4              See if OPEN input or output
         BNZ   NOTU               For output, skip adding RDW bytes
         LA    R1,4(,R1)          Add four for fake RECFM=U RDW
NOTU     DS    0H
         ST    R1,0(,R8)          Return record length back to caller
         TM    DCBRECFM,DCBRECU   See if RECFM=U
         BO    SETRECU            Is RECFM=U, go find input or output?
         TM    DCBRECFM,DCBRECV   See if RECFM=V
         BO    SETRECV            Is RECFM=V, go return "1" to caller
         B     SETRECF            Is RECFM=F, go return "0" to caller
* RECFM=U on input  will tell caller that it is RECFM=V
* RECFM=U on output will tell caller that it is RECFM=F
SETRECU  DS    0H
         LTR   R4,R4              See if OPEN input or output
         BNZ   SETRECF            Is for output, go to set RECFM=F
*        BZ    SETRECV            Else is for input, go to set RECFM=V
SETRECV  DS    0H
         LA    R1,1               Pass RECFM V to caller
         B     SETRECFM           Go to set RECFM=V
SETRECF  DS    0H
         LA    R1,0               Pass RECFM F to caller
*        B     SETRECFM           Go to set RECFM=F
SETRECFM DS    0H
         ST    R1,0(,R5)          Pass either RECFM F or V to caller
*        B     RETURNOP
*
RETURNOP DS    0H
         LR    R1,R13
         L     R13,SAVEAREA+4
         FREEMAIN RU,LV=WORKLEN,A=(1),SP=SUBPOOL
*
         LR    R15,R2             Return neg.RC or GETMAINed area addr.
         RETURN (14,12),RC=(15)   Return to caller
*
* This is not executed directly, but copied into 24-bit storage
ENDFILE  LA    R6,1               Indicate @@AREAD reached end-of-file
         BR    R14                Return to instruction after the GET
EOFRLEN  EQU   *-ENDFILE
*
         LTORG
INDCB    DCB   MACRF=R,DSORG=PS   If member name, will be changed to PO
INDCBLN  EQU   *-INDCB
F32760   DC    F'32760'           Constant for compare
F65536   DC    F'65536'           Maximum VBS record GETMAIN length
OUTDCB   DCB   MACRF=PL,DSORG=PS
OUTDCBLN EQU   *-OUTDCB
*
READDUM  READ  NONE,              Read record Data Event Control Block C
               SF,                Read record Sequential Forward       C
               ,       (R2),      Read record DCB address              C
               ,       (R4),      Read record input buffer             C
               ,       (R5),      Read BLKSIZE or 256 for PDS.DirectoryC
               MF=L               List type MACRO
READLEN  EQU   *-READDUM
*
*
* CAMDUM CAMLST SEARCH,DSNAME,VOLSER,DSCB+44
CAMDUM   CAMLST SEARCH,*-*,*-*,*-*
CAMLEN   EQU   *-CAMDUM           Length of CAMLST Template
*
*
         ENTRY @@AREAD
@@AREAD  EQU   *
         SAVE  (14,12),,@@AREAD
         LR    R12,R15
         USING @@AREAD,R12
         L     R2,0(,R1)          R2 contains GETMAINed address/handle
         USING IHADCB,R2
         L     R3,4(,R1)  R3 points to where to store record pointer
         GETMAIN RU,LV=WORKLEN,SP=SUBPOOL
         ST    R13,4(,R1)
         ST    R1,8(,R13)
         LR    R13,R1
         USING WORKAREA,R13
*
         SLR   R6,R6              Clear default end-of-file indicator
         L     R4,BUFFADDR        Load address of input buffer
         ICM   R5,B'1111',BUFFCURR  Load address of next record
         BNZ   DEBLOCK            Block in memory, go de-block it
         USING IHADCB,R2
         TM    DCBRECFM,DCBRECU   See if RECFM=U
         BNO   READ               Not RECFM=U, go read a block
         LA    R4,4(,R4)          Read RECFM=U four bytes into buffer
         L     R5,BLKSIZE         Load block size to read
READ     DS    0H
*
         AIF   ('&SYS' NE 'S380').N380RD1
         CALL  @@SETM24
.N380RD1 ANOP
*
         READ  DECB,              Read record Data Event Control Block C
               SF,                Read record Sequential Forward       C
               (R2),              Read record DCB address              C
               (R4),              Read record input buffer             C
               (R5),              Read BLKSIZE or 256 for PDS.DirectoryC
               MF=E               Execute a MF=L MACRO
         CHECK DECB               Wait for READ to complete
*                                 If EOF, R6 will be set to F'1'
         AIF   ('&SYS' NE 'S380').N380RD2
         CALL  @@SETM31
.N380RD2 ANOP
*
         LTR   R6,R6              See if end of input data set
         BNZ   READEOD            Is end, go return to caller
* If RECFM=FB or U, store BUFFADDR in BUFFCURR
* If RECFM=V, VB, VBS, etc. store BUFFADDR+4 in BUFFCURR
         LR    R5,R4              Copy buffer address to init BUFFCURR
         TM    DCBRECFM,DCBRECU   See if RECFM=U
         BO    NOTV               Is RECFM=U, so not RECFM=V
         TM    DCBRECFM,DCBRECV   See if RECFM=V, VB, VBS, etc.
         BNO   NOTV               Is not RECFM=V, so skip address bump
         LA    R5,4(,R4)          Bump buffer address past BDW
NOTV     DS    0H
* Subtract residual from BLKSIZE, add BUFFADDR, store as BUFFEND
         ST    R5,BUFFCURR        Indicate data available
         L     R8,DECIOBPT        Load address of Input/Output Block
         USING IOBSTDRD,R8        Give assembler IOB base
         SLR   R7,R7              Clear residual amount work register
         ICM   R7,B'0011',IOBCSW+5  Load residual count
         DROP  R8                 Don't need IOB address base anymore
         L     R8,BLKSIZE         Load maximum block size
         SLR   R8,R7              Find block size read in
         LR    R7,R8              Save size of block read in
         AL    R8,BUFFADDR        Find address after block read in
         ST    R8,BUFFEND         Save address after end of input block
*
DEBLOCK  DS    0H
*        R4 has address of block buffer
*        R5 has address of current record
*        If RECFM=U, then R7 has size of block read in
         TM    DCBRECFM,DCBRECU   Is data set RECFM=U
         BO    DEBLOCKU           Is RECFM=U, go deblock it
         TM    DCBRECFM,DCBRECF   Is data set RECFM=F, FB, etc.
         BO    DEBLOCKF           Is RECFM=Fx, go deblock it
*
* Must be RECFM=V, VB, VBS, VS, VA, VM, VBA, VBM, VSA, VSM, VBSA, VBSM
*  VBS SDW ( Segment Descriptor Word ):
*  REC+0 length 2 is segment length
*  REC+2 0 is record not segmented
*  REC+2 1 is first segment of record
*  REC+2 2 is last seqment of record
*  REC+2 3 is one of the middle segments of a record
*        R5 has address of current record
         CLI   2(R5),0            See if a spanned record segment
         BE    DEBLOCKV           Not spanned, go deblock as RECFM=V
         L     R6,VBSADDR         Load address of the VBS record area
         CLI   2(R5),1            See if first spanned record segment
         BE    DEFIRST            First segment of rec., go start build
         CLI   2(R5),3            If a middle of spanned record segment
         BE    DEMID              A middle segment of rec., go continue
*        CLI   2(R5),3            See if last spanned record segment
*        BE    DELAST             Last segment of rec., go complete rec
* DELAST   DS    0H
*        R5 has address of last record segment
*        R6 has address of VBS record area
         SLR   R7,R7              Clear work register
         ICM   R7,B'0011',0(R5)   Load length of record segment
         SH    R7,H4              Only want length of data
         SLR   R8,R8              Clear work register
         ICM   R8,B'0011',0(R6)   Load length of record so far
         AR    R8,R7              Find length of complete record
         STH   R8,0(,R6)          Store complete record length
         L     R9,VBSCURR         Load old VBS current location
         AR    R9,R8              See if past end of VBS record buffer
         CL    R9,VBSEND          See if past end of VBS record buffer
         BH    ABEND              Record too long, go abend
         L     R8,VBSCURR         Location to put the record seqment
         ST    R9,VBSCURR         Save new VBS next data address
         LR    R9,R7              Length of data to move
         LA    R10,4(,R5)         Location of input record data
         LR    R11,R7             Length of data to move
         MVCL  R8,R10             Move record segment to VBS rec.area
         DC    H'0'               x
*        If end of block, zero BUFFCURR
*        If another record, bump BUFFCURR address
         DC    H'0'               x
         DC    H'0'               x
         DC    H'0'               x
         DC    H'0'               x
         DC    H'0'               x
         DC    H'0'               x
         DC    H'0'               x
         LR    R5,R6              Load address of completed record
         B     RECBACK            Go store the record addr. for return
DEFIRST  DS    0H
* R5 has address of last record segment
* R6 has address of VBS record area
* Most of DEFIRST is the same as DEMID
         XC    2(4,R6),0(R6)      Clear VBS record area RDW
         LA    R7,4               Load four to setup RDW
         STC   R7,1(,R6)          Set length of RDW in RDW
         AL    R7,VBSADDR         Find address to put first data
         ST    R7,VBSCURR         Save new VBS new data address
DEMID    DS    0H
         SLR   R7,R7              Clear work register
         ICM   R7,B'0011',0(R5)   Load length of record segment
         SH    R7,H4              Only want length of data
         SLR   R8,R8              Clear work register
         ICM   R8,B'0011',0(R6)   Load length of record so far
         AR    R8,R7              Find length of complete record
         STH   R8,0(,R6)          Store complete record length
         L     R9,VBSCURR         Load old VBS current location
         AR    R9,R8              See if past end of VBS record buffer
         CL    R9,VBSEND          See if past end of VBS record buffer
         BH    ABEND              Record too long, go abend
         L     R8,VBSCURR         Location to put the record seqment
         ST    R9,VBSCURR         Save new VBS next data address
         LR    R9,R7              Length of data to move
         LA    R10,4(,R5)         Location of input record data
         LR    R11,R7             Length of data to move
         MVCL  R8,R10             Move record segment to VBS rec.area
         WTO   'RECFM=VBS is not supported yet',ROUTCDE=11
         DC    H'0'               x
         DC    H'0'               x
*        If end of block, zero BUFFCURR
*        If another record, bump BUFFCURR address
         DC    H'0'               x
*       Return to READ to get next seqment
         DC    H'0'               x
         DC    H'0'               x
         DC    H'0'               x
         DC    H'0'               x
         DC    H'0'               x
* ELSE add record length to BUFFCURR
* If BUFFCURR equal BUFFEND, zero BUFFCURR
* Move record to VBS at VBSCURR, add to VBSADDR
* If record incomplete, return to READ
* Load VRSADDR into R5            x
* zero VBSCURR                    x
* Branch to RECBACK               x
*                                 x
*
DEBLOCKU DS    0H
* If RECFM=U, a block is a variable record
*        R4 has address of block buffer
*        R7 has size of block read in
         LA    R7,4(,R7)          Add four to block size for fake RDW
         STH   R7,0(,R4)          Store variable RDW for RECFM=U
         LR    R5,R4              Indicate start of buffer is record
         XC    BUFFCURR,BUFFCURR  Indicate no next record in block
         B     RECBACK            Go store the record addr. for return
*
DEBLOCKV DS    0H
* If RECFM=V, bump address RDW size
*        R5 has address of current record
         SLR   R7,R7              Clear DCB-LRECL work register
         ICM   R7,B'0011',0(R5)   Load RECFM=V RDW length field
         AR    R7,R5              Find the next record address
* If address=BUFFEND, zero BUFFCURR
         CL    R7,BUFFEND         Is it off end of block?
         BL    SETCURR            Is not off, go store it
         LA    R7,0               Clear the next record address
         B     SETCURR            Go store next record addr.
*
DEBLOCKF DS    0H
* If RECFM=FB, bump address by lrecl
*        R5 has address of current record
         L     R7,LRECL           Load RECFM=F DCB LRECL
         AR    R7,R5              Find the next record address
* If address=BUFFEND, zero BUFFCURR
         CL    R7,BUFFEND         Is it off end of block?
         BL    SETCURR            Is not off, go store it
         LA    R7,0               Clear the next record address
SETCURR  DS    0H
         ST    R7,BUFFCURR        Store the next record address
*        BNH   RECBACK            Go store the record addr. for return
RECBACK  DS    0H
         ST    R5,0(,R3)          Store record address for caller
READEOD  DS    0H
         LR    R1,R13             Save temp.save area addr.for FREEMAIN
         L     R13,SAVEAREA+4     Restore Caller's save area address
         FREEMAIN RU,LV=WORKLEN,A=(1),SP=SUBPOOL  Free temp.save area
         LR    R15,R6             Set return code 1=EOF or 0=not-eof
         RETURN (14,12),RC=(15)   Return to caller
*
ABEND    DS    0H
         WTO   'MVSSUPA - @@AREAD - encountered VBS record too long',  c
               ROUTCDE=11         Send to programmer and listing
         ABEND 1234,DUMP          Abend U1234 and allow a dump
*
         LTORG                    In case someone adds literals
*
H4       DC    H'4'               Constant for subtraction
*
         ENTRY @@AWRITE
@@AWRITE EQU   *
         SAVE  (14,12),,@@AWRITE
         LR    R12,R15
         USING @@AWRITE,R12
         L     R2,0(,R1)          R2 contains GETMAINed address
         L     R3,4(,R1)          R3 points to the record address
         GETMAIN RU,LV=WORKLEN,SP=SUBPOOL
         ST    R13,4(,R1)
         ST    R1,8(,R13)
         LR    R13,R1
         USING WORKAREA,R13
*
         AIF   ('&SYS' NE 'S380').N380WR1
         CALL  @@SETM24
.N380WR1 ANOP
*
         PUT   (R2)
*
         AIF   ('&SYS' NE 'S380').N380WR2
         CALL  @@SETM31
.N380WR2 ANOP
*
         ST    R1,0(,R3)
         LR    R1,R13
         L     R13,SAVEAREA+4
         FREEMAIN RU,LV=WORKLEN,A=(1),SP=SUBPOOL
         RETURN (14,12),RC=0
*
*
*
         ENTRY @@ACLOSE
@@ACLOSE EQU   *
         SAVE  (14,12),,@@ACLOSE
         LR    R12,R15
         USING @@ACLOSE,R12
         L     R2,0(,R1)          R2 contains GETMAINed address/handle
         GETMAIN RU,LV=WORKLEN,SP=SUBPOOL
         ST    R13,4(,R1)
         ST    R1,8(,R13)
         LR    R13,R1
         USING WORKAREA,R13
*
         ICM   R1,B'1111',VBSADDR  Load VBS record area
         BZ    FREEBUFF           No area, skip free of it
         L     R0,VBSEND          Load address past end of VBS area
         SLR   R0,R1              Calculate size of VBS record area
         FREEMAIN RU,LV=(0),A=(1),SP=SUBPOOL  Free VBS record area
FREEBUFF DS    0H
         ICM   R1,B'1111',BUFFADDR  Load input buffer address
         BZ    CLOSE              No area, skip free of it
         L     R3,BLKSIZE         Load the BLKSIZE for buffer size
         LA    R0,4(,R3)          Add 4 bytes for RECFM=U
         FREEMAIN RU,LV=(0),A=(1),SP=SUBPOOL  Free input buffer
CLOSE    DS    0H
         MVI   OPENCLOS,X'80'     Initialize MODE=24 OPEN/CLOSE list
         CLOSE ((R2)),MF=(E,OPENCLOS)
         TM    DCBMACR1,DCBMRRD   See if using MACRF=R, no dynamic buff
         BO    NOPOOL             Is MACRF=R, skip FREEPOOL
         FREEPOOL ((R2))
NOPOOL   DS    0H
         FREEMAIN RU,LV=ZDCBLEN,A=(R2),SP=SUBPOOL
*
         LR    R1,R13
         L     R13,SAVEAREA+4
         FREEMAIN RU,LV=WORKLEN,A=(1),SP=SUBPOOL
         RETURN (14,12),RC=0
         LTORG
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  GETM - GET MEMORY
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
         ENTRY @@GETM
@@GETM   EQU   *
         SAVE  (14,12),,@@GETM
         LR    R12,R15
         USING @@GETM,R12
*
         L     R2,0(,R1)
         AIF ('&COMP' NE 'GCC').GETMC
* THIS LINE IS FOR GCC
         LR    R3,R2
         AGO   .GETMEND
.GETMC   ANOP
* THIS LINE IS FOR C/370
         L     R3,0(,R2)
.GETMEND ANOP
         LR    R4,R3
         LA    R3,16(,R3)
*
         AIF   ('&SYS' NE 'S380').N380GM1
         L     R1,=X'04100000'
         AGO   .N380GM2
.N380GM1 ANOP
         GETMAIN RU,LV=(R3),SP=SUBPOOL
.N380GM2 ANOP
*
* WE STORE THE AMOUNT WE REQUESTED FROM MVS INTO THIS ADDRESS
         ST    R3,0(R1)
* AND JUST BELOW THE VALUE WE RETURN TO THE CALLER, WE SAVE
* THE AMOUNT THEY REQUESTED
         ST    R4,12(R1)
         A     R1,=F'16'
         LR    R15,R1
*
RETURNGM DS    0H
         RETURN (14,12),RC=(15)
         LTORG
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  FREEM - FREE MEMORY
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
         ENTRY @@FREEM
@@FREEM  EQU   *
         SAVE  (14,12),,@@FREEM
         LR    R12,R15
         USING @@FREEM,R12
*
         L     R2,0(,R1)
         S     R2,=F'16'
         L     R3,0(,R2)
*
         AIF   ('&SYS' NE 'S380').N380FM1
         LA    R15,0
         AGO   .N380FM2
.N380FM1 ANOP
         FREEMAIN RU,LV=(R3),A=(R2),SP=SUBPOOL
.N380FM2 ANOP
*
RETURNFM DS    0H
         RETURN (14,12),RC=(15)
         LTORG
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  GETCLCK - GET THE VALUE OF THE MVS CLOCK TIMER AND MOVE IT TO AN
*  8-BYTE FIELD.  THIS 8-BYTE FIELD DOES NOT NEED TO BE ALIGNED IN
*  ANY PARTICULAR WAY.
*
*  E.G. CALL 'GETCLCK' USING WS-CLOCK1
*
*  THIS FUNCTION ALSO RETURNS THE NUMBER OF SECONDS SINCE 1970-01-01
*  BY USING SOME EMPERICALLY-DERIVED MAGIC NUMBERS
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
         ENTRY @@GETCLK
@@GETCLK EQU   *
         SAVE  (14,12),,@@GETCLK
         LR    R12,R15
         USING @@GETCLK,R12
*
         L     R2,0(,R1)
         STCK  0(R2)
         L     R4,0(,R2)
         L     R5,4(,R2)
         SRDL  R4,12
         SL    R4,=X'0007D910'
         D     R4,=F'1000000'
         SL    R5,=F'1220'
         LR    R15,R5
*
RETURNGC DS    0H
         RETURN (14,12),RC=(15)
         LTORG
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  SETM24 - Set AMODE to 24
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
         ENTRY @@SETM24
@@SETM24 EQU   *
         SAVE  (14,12),,@@SETM24
         LR    R12,R15
         USING @@SETM24,R12
*
         L     R6,=A(LAB24)
         N     R6,=X'7FFFFFFF'
         DC    X'0B06'            BSM R0,R6
LAB24    DS    0H
         LA    R15,0
*
RETURN24 DS    0H
         RETURN (14,12),RC=(15)
         LTORG
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  SETM31 - Set AMODE to 31
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
         ENTRY @@SETM31
@@SETM31 EQU   *
         SAVE  (14,12),,@@SETM31
         LR    R12,R15
         USING @@SETM31,R12
*
         L     R6,=A(LAB31)
         O     R6,=X'80000000'
         DC    X'0B06'            BSM R0,R6
LAB31    DS    0H
         LA    R15,0
*
RETURN31 DS    0H
* We can't use the RETURN macro because R14 is unreliable
* As the BALR if done from 24-bit mode has corrupted the
* high byte.
         L     14,12(13,0)
         N     14,=X'00FFFFFF'
         LM    0,12,20(13)
         BR    14         
         LTORG
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  SAVER - SAVE REGISTERS AND PSW INTO ENV_BUF
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
         ENTRY @@SAVER
@@SAVER EQU   *
*
         SAVE  (14,12),,@@SAVER    * SAVE REGS AS NORMAL
         LR    R12,R15
         USING @@SAVER,12
         L     R1,0(,R1)           * ADDRESS OF ENV TO R1
         L     R2,@@MANSTK         * R2 POINTS TO START OF STACK
         L     R3,@@MANSTK+4       * R3 HAS LENGTH OF STACK
         LR    R5,R3               * AND R5
         LR    R9,R1               * R9 NOW CONTAINS ADDRESS OF ENV
* GET A SAVE AREA
         GETMAIN RU,LV=(R3),SP=SUBPOOL
         ST    R1,0(R9)            * SAVE IT IN FIRST WORK OF ENV
         ST    R5,4(R9)            * SAVE LENGTH IN SECOND WORD OF ENV
         ST    R2,8(R9)            * NOTE WHERE WE GOT IT FROM
         ST    R13,12(R9)          * AND R13
         LR    R4,R1               * AND R4
         MVCL  R4,R2               * COPY SETJMP'S SAVE AREA TO ENV
*        STM   R0,R15,0(R1)               SAVE REGISTERS
*        BALR  R15,0                     GET PSW INTO R15
*        ST    R15,64(,R1)                SAVE PSW
*
RETURNSR DS    0H
         SR    R15,R15              * CLEAR RETURN CODE
         RETURN (14,12),RC=(15)
         ENTRY   @@MANSTK
@@MANSTK DS    2F
         LTORG
*
*
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  LOADR - LOAD REGISTERS AND PSW FROM ENV_BUF
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
         ENTRY @@LOADR
@@LOADR EQU   *
*
         BALR  R12,R0
         USING *,12
         L     R1,0(,R1)          * R1 POINTS TO ENV
         L     R2,8(,R1)          * R2 POINTS TO STACK
         L     R3,4(,R1)          * R3 HAS HOW LONG
         LR    R5,R3              * AS DOES R5
         L     R6,24(,R1)         * R6 HAS RETURN CODE
         L     R4,0(,R1)          * OUR SAVE AREA
         L     R13,12(,R1)        * GET OLD STACK POINTER
         MVCL  R2,R4              * AND RESTORE STACK
         ST    R6,24(,R1)         * SAVE VAL IN ENV
         L     R6,=F'1'
         ST    R6,20(R1)          * AND SET LONGJ TO 1.
         FREEMAIN RU,LV=(R3),A=(R4),SP=SUBPOOL
*        L     R14,16(R1)          * AND RETURN ADDRESS
*        B     RETURNSR            * AND BACK INTO SETJMP
*        L     R15,64(,R1)                RESTORE PSW
*        LM    R0,R15,0(R1)               RESTORE REGISTERS
*        BR    R15                        JUMP TO SAVED PSW
*
RETURNLR DS    0H
         SR    R15,R15            * CLEAR RETURN CODE
         RETURN (14,12),RC=(15)
         LTORG
*
         IEZIOB                   Input/Output Block
*
*
WORKAREA DSECT
SAVEAREA DS    18F
WORKLEN  EQU   *-WORKAREA
*
         DCBD  DSORG=PS,DEVD=DA   Map Data Control Block
         ORG   IHADCB             Overlay the DCB DSECT
ZDCBAREA DS    0H
         DS    CL(INDCBLN)
         DS    CL(OUTDCBLN)
OPENCLOS DS    F                  OPEN/CLOSE parameter list
         DS    0H
EOFR24   DS    CL(EOFRLEN)
         IHADECB DSECT=NO         Data Event Control Block
BLKSIZE  DS    F                  Save area for input DCB BLKSIZE
LRECL    DS    F                  Save area for input DCB LRECL
BUFFADDR DS    F                  Location of the BLOCK Buffer
BUFFEND  DS    F                  Address after end of current block
BUFFCURR DS    F                  Current record in the buffer
VBSADDR  DS    F                  Location of the VBS record build area
VBSEND   DS    F                  Addr. after end VBS record build area
VBSCURR  DS    F                  Location to store next byte
JFCBPTR  DS    F
JFCB     DS    0F
         IEFJFCBN LIST=YES        SYS1.AMODGEN JOB File Control Block
CAMLST   DS    XL(CAMLEN)         CAMLST for OBTAIN to get VTOC entry
* Format 1 Data Set Control Block
DSCB     DS    0F
         IECSDSL1 (1)             Map the Format 1 DSCB
DSCBCCHH DS    CL5                CCHHR of DSCB returned by OBTAIN
         DS    CL47               Rest of OBTAIN's 148 byte work area
ZDCBLEN  EQU   *-ZDCBAREA
*
*
         END
