gccmvs -DUSE_MEMMGR -S -I . -I ../pdpclib ../pdpclib/start.c
gccmvs -DUSE_MEMMGR -S -I . -I ../pdpclib ../pdpclib/stdio.c
gccmvs -DUSE_MEMMGR -S -I . -I ../pdpclib ../pdpclib/stdlib.c
gccmvs -DUSE_MEMMGR -S -I . -I ../pdpclib ../pdpclib/ctype.c
gccmvs -DUSE_MEMMGR -S -I . -I ../pdpclib ../pdpclib/string.c
gccmvs -DUSE_MEMMGR -S -I . -I ../pdpclib ../pdpclib/time.c
gccmvs -DUSE_MEMMGR -S -I . -I ../pdpclib ../pdpclib/errno.c
gccmvs -DUSE_MEMMGR -S -I . -I ../pdpclib ../pdpclib/assert.c
gccmvs -DUSE_MEMMGR -S -I . -I ../pdpclib ../pdpclib/locale.c
gccmvs -DUSE_MEMMGR -S -I . -I ../pdpclib ../pdpclib/math.c
gccmvs -DUSE_MEMMGR -S -I . -I ../pdpclib ../pdpclib/setjmp.c
gccmvs -DUSE_MEMMGR -S -I . -I ../pdpclib ../pdpclib/signal.c
gccmvs -DUSE_MEMMGR -S -I . -I ../pdpclib ../pdpclib/__memmgr.c
gccmvs -DUSE_MEMMGR -S -I . -I ../pdpclib pdosload.c

m4 -I . -I ../pdpclib pdosload.m4 >pdosload.jcl
call runmvs pdosload.jcl output.txt none pdosload.txt binary ascii
hdtofil pdosload.bin <pdosload.txt
echo PDOS00 3390-1 * pdosload.bin >ctl.txt
del pdos00.199
dasdload -bz2 ctl.txt pdos00.199
copy pdos00.199 \mvs380\dasd
call startmvs ipl1b9
