Welcome to PDOS/390 (and friends).

This distribution comes with the PDOS operating system
installed on a 3390. You will need to dump the disk image,
restore it to a real 3390, then zap the SYS1.PDOS dataset
at location 7FC.  Note that there is an eyecatcher
"ZAPCONSL" just before the location.

0007E0  00000000 00000000 00000000 00000000   00000000 E9C1D7C3 D6D5E2D3 00000000   *....................ZAPCONSL....*
000800  47F0F00C 067C7CC3 D9E3F000 90ECD00C   18CF58FD 004C50DF 000450FD 000818DF   *.00..@@CRT0..........<&...&.....*

The following JCL will do that:

//ZAP      JOB CLASS=C,REGION=0K
//*
//* zap a dataset
//*
//ZAP      EXEC PGM=IMASPZAP
//SYSPRINT DD  SYSOUT=*
//SYSLIB   DD  DSN=SYS1.PDOS,DISP=SHR,UNIT=SYSALLDA,VOL=SER=PDOS00
//SYSIN    DD  *
 CCHHR 0001000001
 VER 07FC 00000000
 REP 07FC 00010038
 ABSDUMPT ALL
/*
//*
//

(the example sets the subchannel to x'10038')

Options for dumping the data would be to run ADRDSSU on a Hercules
system, or there is also a CCKDDUMP/CCKDLOAD.

I'm hoping that DSSDUMP can also be enhanced to do a physical
dump, but at time of writing, Gerhard is waiting on some
sample dumps to be made available. Maybe dumping this data
(uncompressed) with ADRDSSU would do.
