gccmvs -DUSE_MEMMGR -DS380 -S -I . -I ../pdpclib ../pdpclib/start.c
gccmvs -DUSE_MEMMGR -DS380 -S -I . -I ../pdpclib ../pdpclib/stdio.c
gccmvs -DUSE_MEMMGR -DS380 -S -I . -I ../pdpclib ../pdpclib/stdlib.c
gccmvs -DUSE_MEMMGR -DS380 -S -I . -I ../pdpclib ../pdpclib/ctype.c
gccmvs -DUSE_MEMMGR -DS380 -S -I . -I ../pdpclib ../pdpclib/string.c
gccmvs -DUSE_MEMMGR -DS380 -S -I . -I ../pdpclib ../pdpclib/time.c
gccmvs -DUSE_MEMMGR -DS380 -S -I . -I ../pdpclib ../pdpclib/errno.c
gccmvs -DUSE_MEMMGR -DS380 -S -I . -I ../pdpclib ../pdpclib/assert.c
gccmvs -DUSE_MEMMGR -DS380 -S -I . -I ../pdpclib ../pdpclib/locale.c
gccmvs -DUSE_MEMMGR -DS380 -S -I . -I ../pdpclib ../pdpclib/math.c
gccmvs -DUSE_MEMMGR -DS380 -S -I . -I ../pdpclib ../pdpclib/setjmp.c
gccmvs -DUSE_MEMMGR -DS380 -S -I . -I ../pdpclib ../pdpclib/signal.c
gccmvs -DUSE_MEMMGR -DS380 -S -I . -I ../pdpclib ../pdpclib/__memmgr.c
gccmvs -DUSE_MEMMGR -DS380 -S -I . -I ../pdpclib pload.c
gccmvs -DUSE_MEMMGR -DS380 -S -I . -I ../pdpclib pdos.c
gccmvs -DUSE_MEMMGR -DS380 -S -I . -I ../pdpclib pcomm.c
gccmvs -DUSE_MEMMGR -DS380 -S -I . -I ../pdpclib world.c

rem when globally changing, change the DS3x0 to whatever
rem and then change the single conf file to the right one

copy \mvs380\conf\mvs380.conf herc.cnf
m4 -I . -I ../pdpclib pdos.m4 >pdos.jcl
call runmvs pdos.jcl output.txt none pload.zip
unzip -o pload
copy pload.txt pload.bin
copy pdos.txt pdos.bin
copy pdosin.txt pdos.in
copy pcomm.txt pcomm.bin
copy pcommin.txt pcomm.in
copy world.txt world.bin
copy world2.txt world2.bin
copy sample.txt sample.c
echo PDOS00 3390-1 * separate >ctl.txt
echo PLOAD.SYS SEQ pload.bin TRK 15 1 0 PS U 0 18452 >>ctl.txt
echo SYSVTOC VTOC TRK 10 >>ctl.txt
echo PDOS.SYS SEQ pdos.bin TRK 15 1 0 PS U 0 18452 >>ctl.txt
echo CONFIG.SYS SEQ pdos.in TRK 3 1 0 PS U 0 18452 >>ctl.txt
echo COMMAND.EXE SEQ pcomm.bin TRK 15 1 0 PS U 0 18452 >>ctl.txt
echo AUTOEXEC.BAT SEQ pcomm.in TRK 3 1 0 PS U 0 18452 >>ctl.txt
echo WORLD.EXE SEQ world.bin TRK 15 1 0 PS U 0 18452 >>ctl.txt
echo WORLD2.EXE SEQ world2.bin TRK 15 1 0 PS U 0 18452 >>ctl.txt
echo SAMPLE.C SEQ sample.c TRK 3 1 0 PS U 0 18452 >>ctl.txt
del pdos00.199
dasdload -bz2 ctl.txt pdos00.199
copy pdos00.199 \mvs380\dasd
copy \mvs380\conf\mvs380_380.conf \mvs380\conf\mvs380.conf
call startmvs ipl1b9
copy \mvs380\conf\mvs380_std.conf \mvs380\conf\mvs380.conf
