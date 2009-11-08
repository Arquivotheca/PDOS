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
         COPY  PDPTOP
*
         CSECT
         PRINT NOGEN
         REGS
         MUSVC
SUBPOOL  EQU   0
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  AOPEN- Open a dataset
*
*  Note that under MUSIC, RECFM=F is the only reliable thing. It is
*  possible to use RECFM=V like this:
*  /file myin tape osrecfm(v) lrecl(32756) vol(PCTOMF) old
*  but it is being used outside the normal MVS interface. All this
*  stuff really needs to be rewritten per normal MUSIC coding.
*
*
*  Note - more documentation for this and other I/O functions can
*  be found halfway through the stdio.c file in PDPCLIB.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
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
         L     R4,04(,R1)         R4 is pointer to the MODE.
*                                 0=input 1=output
         L     R4,0(R4)           R4 has the value of mode.
* 08(,R1) has RECFM
* Note that R5 is used as a scratch register
         L     R8,12(,R1)         R8 POINTS TO LRECL
* 16 has blksize
* 20 has asmbuf
         L     R9,24(,R1)         R9 POINTS TO MEMBER NAME (OF PDS)
         LA    R9,00(,R9)         Strip off high-order bit or byte
*
* S/370 can't handle LOC=BELOW
*
         AIF   ('&SYS' NE 'S370').MVS8090  If not S/370 then 380 or 390
         GETMAIN RU,LV=ZDCBLEN,SP=SUBPOOL  No LOC= for S/370
         AGO   .GETOEND
.MVS8090 ANOP  ,                  S/380 or S/390
*         GETMAIN RU,LV=ZDCBLEN,SP=SUBPOOL,LOC=BELOW
         GETMAIN RU,LV=ZDCBLEN,SP=SUBPOOL
.GETOEND ANOP
*
         LR    R2,R1              Addr.of storage obtained to its base
         USING IHADCB,R2          Give assembler DCB area base register
         LR    R0,R2              Load output DCB area address
         LA    R1,ZDCBLEN         Load output length of DCB area
         LA    R10,0              No input location
         LR    R5,R11             Preserve parameter list
         LA    R11,0              Pad of X'00' and no input length
         MVCL  R0,R10             Clear DCB area to binary zeroes
         LR    R11,R5             Restore parameter list
*
* The member name may not be below the line, which may stuff up
* the "FIND" macro, so make sure it is in 24-bit memory.
*
         LTR   R9,R9              See if an address for the member name
         BZ    NOMEM              No member name, skip copying
         MVC   MEMBER24,0(R9)
         LA    R9,MEMBER24
NOMEM    DS    0H
*
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
*
         AIF   ('&OUTM' NE 'M').NMM4
         L     R6,=F'32768'
* Give caller an internal buffer to write to. Below the line!
*
* S/370 can't handle LOC=BELOW
*
         AIF   ('&SYS' NE 'S370').MVT8090  If not S/370 then 380 or 390
         GETMAIN R,LV=(R6),SP=SUBPOOL  No LOC= for S/370
         AGO   .GETOENE
.MVT8090 ANOP  ,                  S/380 or S/390
*         GETMAIN R,LV=(R6),SP=SUBPOOL,LOC=BELOW
         GETMAIN R,LV=(R6),SP=SUBPOOL
.GETOENE ANOP
         ST    R1,ASMBUF
         L     R5,20(,R11)        R5 points to ASMBUF
         ST    R1,0(R5)           save the pointer
* R5 now free again
*
.NMM4    ANOP
         MVC   LRECL+2(2),DCBLRECL  Copy LRECL to a fullword
         MVC   BLKSIZE+2(2),DCBBLKSI  Copy BLKSIZE to a fullword
         L     R6,BLKSIZE         If file is dummy, and a block size
         LTR   R6,R6              of 0, then we will go into a loop
         BNZ   WNZERO             unless we can fudge it
         LA    R6,1
         ST    R6,BLKSIZE
         ST    R6,LRECL
WNZERO   DS    0H
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
         LA    R1,2
         B     SETRECFM
*        LTR   R4,R4              See if OPEN input or output
*        BNZ   SETRECF            Is for output, go to set RECFM=F
*        BZ    SETRECV            Else is for input, go to set RECFM=V
SETRECV  DS    0H
         LA    R1,1               Pass RECFM V to caller
         B     SETRECFM           Go to set RECFM=V
SETRECF  DS    0H
         LA    R1,0               Pass RECFM F to caller
*        B     SETRECFM           Go to set RECFM=F
SETRECFM DS    0H
         L     R5,8(,R11)         Point to RECFM
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
*
* OUTDCB changes depending on whether we are in LOCATE mode or
* MOVE mode
         AIF   ('&OUTM' NE 'L').NLM1
OUTDCB   DCB   MACRF=PL,DSORG=PS
.NLM1    ANOP
         AIF   ('&OUTM' NE 'M').NMM1
OUTDCB   DCB   MACRF=PM,DSORG=PS
.NMM1    ANOP
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
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  AREAD - Read from an open dataset
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
         ENTRY @@AREAD
@@AREAD  EQU   *
         SAVE  (14,12),,@@AREAD
         LR    R12,R15
         USING @@AREAD,R12
         L     R2,0(,R1)          R2 contains GETMAINed address/handle
         USING IHADCB,R2
         USING ZDCBAREA,R2
*
* R3 is a scratch register
         L     R3,4(,R1)     R3 points to where to store record pointer
         ST    R3,RDRECPTR        Save pointer away
         L     R3,8(,R1)     R3 points to where to store read length
         ST    R3,RDLENPTR        Save pointer away
*
*        GETMAIN RU,LV=WORKLEN,SP=SUBPOOL
         LA    R1,SAVEADCB
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
*                                 If EOF, R6 will be set to F'1'
         CHECK DECB               Wait for READ to complete
*
* Some debugging to see what MUSIC actually gives us, which
* isn't pretty.
*         LH    R8,DCBLRECL
*         LH    R9,DCBBLKSI
*         L     R10,0(R4)
*         L     R11,4(R4)
*         L     R12,8(R4)
*         DC    H'0'
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
* For MUSIC, get the length of the block from the BDW, which is
* the only semi-reliable place there seems to be.
         LH    R7,0(,R4)
         LR    R8,R7
*         DC    H'0'
         LA    R5,4(,R4)          Bump buffer address past BDW
         ST    R5,BUFFCURR        Indicate data available
         AL    R8,BUFFADDR        Find address after block read in
         ST    R8,BUFFEND         Save address after end of input block
         B     DEBLOCK
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
*
* Unfortunately MUSIC gives pretty bad values here. It is
* not providing the length read anywhere. That's why we
* have special processing for V previously, where we can
* get the implied length from the data. For RECFM=F, a
* single record is given in response to the block request,
* so that is able to "work"
*         DC    H'0'
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
*         LA    R7,4(,R7)          Add four to block size for fake RDW
         L     R3,RDLENPTR        Where to store record length
         ST    R7,0(,R3)          Save length
*        STH   R7,0(,R4)          Store variable RDW for RECFM=U
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
         L     R3,RDRECPTR
         ST    R5,0(,R3)          Store record address for caller
READEOD  DS    0H
*        LR    R1,R13             Save temp.save area addr.for FREEMAIN
*        L     R13,SAVEAREA+4     Restore Caller's save area address
         L     R13,SAVEADCB+4
*        FREEMAIN RU,LV=WORKLEN,A=(1),SP=SUBPOOL  Free temp.save area
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
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  AWRITE - Write to an open dataset
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
         ENTRY @@AWRITE
@@AWRITE EQU   *
         SAVE  (14,12),,@@AWRITE
         LR    R12,R15
         USING @@AWRITE,R12
         L     R2,0(,R1)          R2 contains GETMAINed address
         L     R3,4(,R1)          R3 points to the record address
         L     R4,8(,R1)          R4 points to the length
         L     R4,0(,R4)          R4 now has actual length
         USING ZDCBAREA,R2
*        GETMAIN RU,LV=WORKLEN,SP=SUBPOOL
         LA    R1,SAVEADCB
         ST    R13,4(,R1)
         ST    R1,8(,R13)
         LR    R13,R1
*        USING WORKAREA,R13
*
         AIF   ('&SYS' NE 'S380').N380WR1
         CALL  @@SETM24
.N380WR1 ANOP
*
         STCM  R4,B'0011',DCBLRECL
*
         AIF   ('&OUTM' NE 'L').NLM2
         PUT   (R2)
.NLM2    ANOP
         AIF   ('&OUTM' NE 'M').NMM2
* In move mode, always use our internal buffer. Ignore passed parm.
         L     R3,ASMBUF
         PUT   (R2),(R3)
.NMM2    ANOP
         AIF   ('&OUTM' NE 'L').NLM3
         ST    R1,0(R3)
.NLM3    ANOP
*
         AIF   ('&SYS' NE 'S380').N380WR2
         CALL  @@SETM31
.N380WR2 ANOP
*
*        LR    R1,R13
*        L     R13,SAVEAREA+4
         L     R13,SAVEADCB+4
*        FREEMAIN RU,LV=WORKLEN,A=(1),SP=SUBPOOL
         RETURN (14,12),RC=0
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  ACLOSE - Close a dataset
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
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
         USING ZDCBAREA,R2
* If we are doing move mode, free internal assembler buffer
         AIF   ('&OUTM' NE 'M').NMM6
         L     R5,ASMBUF
         LTR   R5,R5
         BZ    NFRCL
         L     R6,=F'32768'
         FREEMAIN R,LV=(R6),A=(R5),SP=SUBPOOL
NFRCL    DS    0H
.NMM6    ANOP
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
         LA    R3,8(,R3)
*
* To avoid fragmentation, round up size to 64 byte multiple
*
         A     R3,=A(64-1)
         N     R3,=X'FFFFFFC0'
*
         AIF   ('&SYS' NE 'S380').N380GM1
*         GETMAIN RU,LV=(R3),SP=SUBPOOL,LOC=ANY
* Hardcode the ATL memory area provided by latest MUSIC.
* Note that this function will only work if the C library
* is compiled with MEMMGR option.
         L     R1,=X'02000000'
         AGO   .N380GM2
.N380GM1 ANOP
         GETMAIN RU,LV=(R3),SP=SUBPOOL
.N380GM2 ANOP
*
* WE STORE THE AMOUNT WE REQUESTED FROM MVS INTO THIS ADDRESS
         ST    R3,0(R1)
* AND JUST BELOW THE VALUE WE RETURN TO THE CALLER, WE SAVE
* THE AMOUNT THEY REQUESTED
         ST    R4,4(R1)
         A     R1,=F'8'
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
         S     R2,=F'8'
         L     R3,0(,R2)
*
         AIF   ('&SYS' NE 'S380').N380FM1
* On S/380, nothing to free - using preallocated memory block
*         FREEMAIN RU,LV=(R3),A=(R2),SP=SUBPOOL
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
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  SYSTEM - execute another command
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
         ENTRY @@SYSTEM
@@SYSTEM EQU   *
         SAVE  (14,12),,@@SYSTEM
         LR    R12,R15
         USING @@SYSTEM,R12
         LR    R11,R1
*
         GETMAIN RU,LV=SYSTEMLN,SP=SUBPOOL
         ST    R13,4(,R1)
         ST    R1,8(,R13)
         LR    R13,R1
         LR    R1,R11
         USING SYSTMWRK,R13
*
         MVC   CMDPREF,FIXEDPRF
         L     R2,0(R1)
         CL    R2,=F'200'
         BL    LENOK
         L     R2,=F'200'
LENOK    DS    0H
         STH   R2,CMDLEN
         LA    R4,CMDTEXT
         LR    R5,R2
         L     R6,4(R1)
         LR    R7,R2
         MVCL  R4,R6
         LA    R1,CMDPREF
         SVC   $EXREQ
*
RETURNSY DS    0H
         LR    R1,R13
         L     R13,SYSTMWRK+4
         FREEMAIN RU,LV=SYSTEMLN,A=(1),SP=SUBPOOL
*
         LA    R15,0
         RETURN (14,12),RC=(15)   Return to caller
* For documentation on this fixed prefix, see SVC 221
* documentation.
FIXEDPRF DC    X'7F01E000000000'
         LTORG
SYSTMWRK DSECT ,             MAP STORAGE
         DS    18A           OUR OS SAVE AREA
CMDPREF  DS    CL8           FIXED PREFIX
CMDLEN   DS    H             LENGTH OF COMMAND
CMDTEXT  DS    CL200         COMMAND ITSELF
SYSTEMLN EQU   *-SYSTMWRK    LENGTH OF DYNAMIC STORAGE
         CSECT ,
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  TEXTLC - switch terminal to lower case mode
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
         ENTRY @@TEXTLC
@@TEXTLC EQU   *
         SAVE  (14,12),,@@TEXTLC  Save caller's regs.
         LR    R12,R15
         USING @@TEXTLC,R12
         LR    R11,R1
*
         GETMAIN RU,LV=WORKLEN,SP=SUBPOOL
         ST    R13,4(,R1)
         ST    R1,8(,R13)
         LR    R13,R1
         LR    R1,R11
         USING WORKAREA,R13
*
         LA    R1,LCOPTS
         SVC   $SETOPT
*
RETURNLC DS    0H
         LR    R1,R13
         L     R13,SAVEAREA+4
         FREEMAIN RU,LV=WORKLEN,A=(1),SP=SUBPOOL
*
         LA    R15,0              Return success
         RETURN (14,12),RC=(15)   Return to caller
*
LCOPTS   DC    X'A0'              Constant
         DC    X'01'              Set bit on
         DC    X'01'              Option byte 1 (1-based)
         DC    X'06'              Bit number 6 (0-based)
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  IDCAMS - dummy function to keep MVS happy
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
         ENTRY @@IDCAMS
@@IDCAMS EQU   *
         SAVE  (14,12),,@@IDCAMS
         LR    R12,R15
         USING @@IDCAMS,R12
*
         LA    R15,0
*
         RETURN (14,12),RC=(15)
         LTORG
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  DYNAL - dynamic allocation dummy function to keep MVS happy
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
         ENTRY @@DYNAL
@@DYNAL  EQU   *
         SAVE  (14,12),,@@DYNAL
         LR    R12,R15
         USING @@DYNAL,R12
*
         LA    R15,0
*
         RETURN (14,12),RC=(15)
         LTORG
*
         DROP  R12
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  SETJ - SAVE REGISTERS INTO ENV
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
         ENTRY @@SETJ
         USING @@SETJ,R15
@@SETJ   L     R15,0(R1)          get the env variable
         STM   R0,R14,0(R15)      save registers to be restored
         LA    R15,0              setjmp needs to return 0
         BR    R14                return to caller
         LTORG ,
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  LONGJ - RESTORE REGISTERS FROM ENV
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
         ENTRY @@LONGJ
         USING @@LONGJ,R15
@@LONGJ  L     R2,0(R1)           get the env variable
         L     R15,60(R2)         get the return code
         LM    R0,R14,0(R2)       restore registers
         BR    R14                return to caller
         LTORG ,
*
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
@@SETM24 ICM   R14,8,=X'00'       Sure hope caller is below the line
         BSM   0,R14              Return in amode 24
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*  SETM31 - Set AMODE to 31
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
         ENTRY @@SETM31
         USING @@SETM31,R15
@@SETM31 ICM   R14,8,=X'80'       Set to switch mode
         BSM   0,R14              Return in amode 31
         LTORG ,
*
.NOMODE  ANOP  ,                  S/370 doesn't support MODE switching
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
RDRECPTR DS    F                  Where to store record pointer
RDLENPTR DS    F                  Where to store read length
JFCBPTR  DS    F
JFCB     DS    0F
         IEFJFCBN LIST=YES        SYS1.AMODGEN JOB File Control Block
CAMLST   DS    XL(CAMLEN)         CAMLST for OBTAIN to get VTOC entry
* Format 1 Data Set Control Block
DSCB     DS    0F
         IECSDSL1 (1)             Map the Format 1 DSCB
DSCBCCHH DS    CL5                CCHHR of DSCB returned by OBTAIN
         DS    CL47               Rest of OBTAIN's 148 byte work area
SAVEADCB DS    18F                Register save area for PUT
ASMBUF   DS    A                  Pointer to an area for PUTing data
MEMBER24 DS    CL8
ZDCBLEN  EQU   *-ZDCBAREA
*
         END
