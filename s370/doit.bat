rem theoretically we could have a different C library for all 3
rem of MVS applications, PDOS applications and standalone programs,
rem which would require 3 different conditional compilations.
rem However, PDOS applications are sufficiently close to MVS that
rem a special compilation is not required. In addition, the
rem structure and scope of standalone programs (such as the PDOS
rem operating system itself) don't use most of the C library
rem functions, and the bit that they do use can fit within the
rem same structure so long as the assembler code is changed.
rem That is how we get away without requiring a special version
rem of the C library!

gccmvs -DUSE_MEMMGR -Os -DS390 -S -I . -I ../pdpclib ../pdpclib/start.c
gccmvs -DUSE_MEMMGR -Os -DS390 -S -I . -I ../pdpclib ../pdpclib/stdio.c
gccmvs -DUSE_MEMMGR -Os -DS390 -S -I . -I ../pdpclib ../pdpclib/stdlib.c
gccmvs -DUSE_MEMMGR -Os -DS390 -S -I . -I ../pdpclib ../pdpclib/ctype.c
gccmvs -DUSE_MEMMGR -Os -DS390 -S -I . -I ../pdpclib ../pdpclib/string.c
gccmvs -DUSE_MEMMGR -Os -DS390 -S -I . -I ../pdpclib ../pdpclib/time.c
gccmvs -DUSE_MEMMGR -Os -DS390 -S -I . -I ../pdpclib ../pdpclib/errno.c
gccmvs -DUSE_MEMMGR -Os -DS390 -S -I . -I ../pdpclib ../pdpclib/assert.c
gccmvs -DUSE_MEMMGR -Os -DS390 -S -I . -I ../pdpclib ../pdpclib/locale.c
gccmvs -DUSE_MEMMGR -Os -DS390 -S -I . -I ../pdpclib ../pdpclib/math.c
gccmvs -DUSE_MEMMGR -Os -DS390 -S -I . -I ../pdpclib ../pdpclib/setjmp.c
gccmvs -DUSE_MEMMGR -Os -DS390 -S -I . -I ../pdpclib ../pdpclib/signal.c
gccmvs -DUSE_MEMMGR -Os -DS390 -S -I . -I ../pdpclib ../pdpclib/__memmgr.c
gccmvs -DUSE_MEMMGR -Os -DS390 -S -I . -I ../pdpclib pload.c
gccmvs -DUSE_MEMMGR -Os -DS390 -S -I . -I ../pdpclib pdos.c
gccmvs -DUSE_MEMMGR -Os -DS390 -S -I . -I ../pdpclib pdosutil.c
gccmvs -DUSE_MEMMGR -Os -DS390 -S -I . -I ../pdpclib pcomm.c
gccmvs -DUSE_MEMMGR -O0 -DS390 -S -I . -I ../pdpclib world.c

sleep 1

rem when globally changing, change the DS3x0 to whatever
rem and then change the file in the next line after comments
rem also you will need to change pdptop.mac

copy pdos390.cnf pdos.cnf


rem we did compiles already, but now need to do assembles and
rem links, plus iebcopy unloads, so we use MVS for that

m4 -I . -I ../pdpclib pdos.m4 >pdos.jcl
call runmvs pdos.jcl output.txt none pdos.zip


rem extract everything and put into expected names

unzip -o pdos
copy pload.txt pload.sys
copy pdos.txt pdos.sys
copy pdosin.txt config.sys
copy pcomm.txt pcomm.exe
copy pcommin.txt autoexec.bat
copy world.txt world.exe
copy sample.txt sample.c
copy wtoworld.txt wtoworld.exe
copy diff.txt diff.exe


rem create dummy batch file

echo echo nothing to run >hercauto.bat
del hercauto.zip
zip -0X hercauto hercauto.bat


rem get GCC/PDPCLIB utilities

unzip -o \download\gccpdos


rem build DASD. Put a copy into MVS/380 area for no particular reason

del pdos00.cckd
dasdload -bz2 ctl.txt pdos00.cckd
copy pdos00.cckd %MVS380%\dasd\pdos00.199


rem Try out the new version of PDOS, and remember to manually do the
rem mvsunzip pdpi.zip


set HERCULES_RC=auto_ipl.rc
hercules -f pdos.cnf >hercules.log


rem create package suitable for "shipping"

del pdospkg.zip
zip -9X pdospkg pload.sys pdos.sys config.sys readme.txt
zip -9X pdospkg pcomm.exe autoexec.bat world.exe sample.c 
zip -9X pdospkg wtoworld.exe diff.exe hercauto.zip
zip -9X pdospkg ctl.txt pdos00.cckd pdos.cnf auto*.rc termherc*.rc
zip -9X pdospkg runpdos.bat pdos.bat pdpi.zip
zip -9X pdospkg gcc.exe mvsendec.exe mvsunzip.exe hexdump.exe copyfile.exe

rem Simply unzip the package into c:\pdos or whatever and it's done
