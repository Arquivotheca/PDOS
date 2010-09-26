Welcome to PDOS/390 (and friends).

This distribution comes with the PDOS operating system
installed on a 3390. You will need to dump the 3390-1
disk image (pdos00.199), restore it to a real 3390, 
then zap the SYS1.PDOS dataset at location 7FC.  Note 
that there is an eyecatcher "ZAPCONSL" just before the 
location.

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
system running z/OS, or there is also a CCKDDUMP/CCKDLOAD. I've
never actually used either, as I don't have access to a real system.

I'm hoping that DSSDUMP can also be enhanced to do a physical
dump, but at time of writing, Gerhard is waiting on some
sample dumps to be made available. Maybe dumping this data
(uncompressed) with ADRDSSU would give him what he needs.

Note that PDOS is expecting the console to be a 3215, not a
3270. Since real sites are expected to be using a PC emulator
of some description, or perhaps telnet, it is unclear whether
it is possible to get this to work in the real world, even
assuming some of the programming shortcuts (ie not checking
for device errors) work in the real world.

Here is an example of how to define the disk under Hercules:

01b9      3390      dasd/pdos00.199

Although all the source code for PDOS is provided, the
build script (doit.bat) makes use of some separately-packaged
utilities. Also note that pdptop.mac is copied from pdp390.mac.

Note that currently PDOS is only designed to load a single
load module (PCOMM) and then terminate (which eventuates in
a wait code of 444). PCOMM is a normal MVS load module doing
normal MVS calls, but PDOS only supports a fraction of normal
MVS calls (it's still in design/proof-of-concept stage, basically).


Support/feedback can be obtained here:

http://tech.groups.yahoo.com/group/hercules-os380/
