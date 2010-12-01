* Public domain by Louis Millon
         REGEQU
RECLEN   EQU       R12
TOCIL    CSECT
         USING     *,R3
         BALR      R3,0
         BCTR      R3,0
         BCTR      R3,0
         B         DEBCODE
FILEU    DTFSD     RECFORM=UNDEF,DEVICE=3350,IOAREA1=BUFFER,           *
               BLKSIZE=19069,                                          *
               DEVADDR=SYS000,TYPEFLE=INPUT,RECSIZE=(12),EOFADDR=FIN
CARD     DS        CL80
ESD      DC        X'02'
         DC        C'ESD',CL6' '
         DC        X'00204040000140404040404040400400000000000000'
         DC        CL8'TOTO',XL4'0',X'0'
CSCTSIZE DC        XL3'0'
         DC        CL80' '
LEN      DC        H'0'
END      DC        X'02'
         DC        CL79'END'
TXTCARD  DC        X'02'
         DC        CL4'TXT'
OFF7X    DS        XL3
         DC        CL2' '
LENX     DS        XL2
         DC        CL2' '
         DC        XL2'0002'
PART     DS        CL56
         DS        CL10' '
OFF7     DS        F
DEBCODE  DS        0H
         EXCP      READ
         WAIT      READ
         OPEN      FILEU
         LA        R10,0
         LA        R9,FINSIZE
LOOPSIZE DS        0H
         GET       FILEU,BUFFER
         AR        R10,RECLEN
         LA        R10,4(R10)
         B         LOOPSIZE
FINSIZE  DS        0H
         CLOSE     FILEU
         LA        R10,4(R10)
         STCM      R10,B'0111',CSCTSIZE
         LA        R9,FINFIN
         OPEN      FILEU
         LA        R10,PART1
LOOP1    DS        0H
         MVC       CARD,0(R10)
         EXCP      PUNCH
         WAIT      PUNCH
         LA        R10,80(R10)
         CLC       =F'-1',0(R10)
         BNE       LOOP1
         LR        R15,RECLEN
         LA        R15,4(R15)
         STCM      R15,B'0111',LEN-1
         MVC       CARD,ESD
         EXCP      PUNCH
         WAIT      PUNCH
         XC        OFF7X,OFF7X
LECTURE  DS        0H
         GET       FILEU,BUFFER
         MVC       LENX,=H'4'
         STCM      RECLEN,B'1111',PART
         MVC       CARD,TXTCARD
         SR        R15,R15
         EXCP      PUNCH
         WAIT      PUNCH
         ICM       R15,B'0111',OFF7X
         LA        R15,4(R15)
         STCM      R15,B'0111',OFF7X
         MVC       OFF7,=F'0'
         SR        R15,R15
LOOP0    DS        0H
         LA        R15,56
         CR        R15,RECLEN
         BNH       NO56
         LR        R15,RECLEN
NO56     DS        0H
         STH       R15,LEN
*        L         R15,OFF7
*        STCM      R15,B'0111',OFF7X
         MVC       LENX,LEN
         LA        R15,BUFFER
         A         R15,OFF7
         MVC       PART,0(R15)
         MVC       CARD,TXTCARD
         EXCP      PUNCH
         WAIT      PUNCH
         ICM       R15,B'0111',OFF7X
         AH        R15,LEN
         STCM      R15,B'0111',OFF7X
         L         R15,OFF7
         AH        R15,LEN
         ST        R15,OFF7
         SH        RECLEN,LEN
         CH        RECLEN,=H'1'
         BNL       LOOP0
         B         LECTURE
FINFIN   DS        0H
         MVC       LENX,=H'4'
         STCM      RECLEN,B'1111',PART
         MVC       CARD,TXTCARD
         EXCP      PUNCH
         WAIT      PUNCH
         MVC       CARD,END
         EXCP      PUNCH
         WAIT      PUNCH
         LA        R10,PART2
LOOP2    DS        0H
         MVC       CARD,0(R10)
         EXCP      PUNCH
         WAIT      PUNCH
         LA        R10,80(R10)
         CLC       =F'-1',0(R10)
         BNE       LOOP2
         EOJ
FIN      DS        0H
         BR        R9
PART1    DS        0D
PHASE    DC        CL80' '
         DC        CL80' INCLUDE'
         DC        F'-1'
PART2    DS        0D
         DC        CL80'/*'
         DC        CL80'// EXEC LNKEDT'
         DC        CL80'* $$ PUN DISP=I'
         DC        CL80'/&&'
         DC        CL80'* $$ EOJ'
         DC        F'-1'
PUNCH    CCB       SYSPCH,CCW
CCW      CCW       X'09',CARD,0,80
READ     CCB       SYSIPT,CCWR
CCWR     CCW       X'02',PHASE,0,80
         LTORG
BUFFER   DS        32772C'%'
         END
