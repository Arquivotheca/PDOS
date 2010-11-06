gccmvs -DUSE_MEMMGR -D__PDOS__ -Os -DS390 -S -I . -I ../pdpclib ../pdpclib/start.c
gccmvs -DUSE_MEMMGR -D__PDOS__ -Os -DS390 -S -I . -I ../pdpclib ../pdpclib/stdio.c
gccmvs -DUSE_MEMMGR -D__PDOS__ -Os -DS390 -S -I . -I ../pdpclib ../pdpclib/stdlib.c
gccmvs -DUSE_MEMMGR -D__PDOS__ -Os -DS390 -S -I . -I ../pdpclib ../pdpclib/ctype.c
gccmvs -DUSE_MEMMGR -D__PDOS__ -Os -DS390 -S -I . -I ../pdpclib ../pdpclib/string.c
gccmvs -DUSE_MEMMGR -D__PDOS__ -Os -DS390 -S -I . -I ../pdpclib ../pdpclib/time.c
gccmvs -DUSE_MEMMGR -D__PDOS__ -Os -DS390 -S -I . -I ../pdpclib ../pdpclib/errno.c
gccmvs -DUSE_MEMMGR -D__PDOS__ -Os -DS390 -S -I . -I ../pdpclib ../pdpclib/assert.c
gccmvs -DUSE_MEMMGR -D__PDOS__ -Os -DS390 -S -I . -I ../pdpclib ../pdpclib/locale.c
gccmvs -DUSE_MEMMGR -D__PDOS__ -Os -DS390 -S -I . -I ../pdpclib ../pdpclib/math.c
gccmvs -DUSE_MEMMGR -D__PDOS__ -Os -DS390 -S -I . -I ../pdpclib ../pdpclib/setjmp.c
gccmvs -DUSE_MEMMGR -D__PDOS__ -Os -DS390 -S -I . -I ../pdpclib ../pdpclib/signal.c
gccmvs -DUSE_MEMMGR -D__PDOS__ -Os -DS390 -S -I . -I ../pdpclib ../pdpclib/__memmgr.c
gccmvs -DUSE_MEMMGR -D__PDOS__ -Os -DS390 -S -I . -I ../pdpclib pload.c
gccmvs -DUSE_MEMMGR -D__PDOS__ -Os -DS390 -S -I . -I ../pdpclib pdos.c
gccmvs -DUSE_MEMMGR -D__PDOS__ -Os -DS390 -S -I . -I ../pdpclib pdosutil.c
gccmvs -DUSE_MEMMGR -D__PDOS__ -Os -DS390 -S -I . -I ../pdpclib pcomm.c
gccmvs -DUSE_MEMMGR -D__PDOS__ -O0 -DS390 -S -I . -I ../pdpclib world.c

rem when globally changing, change the DS3x0 to whatever
rem and then change the single conf file to the right one

copy pdos390.cnf pdos.cnf


m4 -I . -I ../pdpclib pdos.m4 >pdos.jcl
call runmvs pdos.jcl output.txt none pdos.zip
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

echo echo nothing to run >hercauto.bat
del hercauto.zip
zip -0X hercauto hercauto.bat

del pdospkg.zip
zip -9X pdospkg pload.sys pdos.sys config.sys 
zip -9X pdospkg pcomm.exe autoexec.bat world.exe sample.c 
zip -9X pdospkg wtoworld.exe diff.exe hercauto.zip
zip -9X pdospkg ctl.txt

unzip -o gccpdos

del pdos00.cckd
dasdload -bz2 ctl.txt pdos00.cckd
copy pdos00.cckd \mvs380\dasd\pdos00.199

set HERCULES_RC=auto_ipl.rc
hercules -f pdos.cnf >hercules.log
