rem Update pdptop.mac if you don't want the default S/380.
rem If choosing S/370 you should also switch off MEMMGR

gccmvs -DUSE_MEMMGR -S -I . start.c
gccmvs -DUSE_MEMMGR -S -I . stdio.c
gccmvs -DUSE_MEMMGR -S -I . stdlib.c
gccmvs -DUSE_MEMMGR -S -I . ctype.c
gccmvs -DUSE_MEMMGR -S -I . string.c
gccmvs -DUSE_MEMMGR -S -I . time.c
gccmvs -DUSE_MEMMGR -S -I . errno.c
gccmvs -DUSE_MEMMGR -S -I . assert.c
gccmvs -DUSE_MEMMGR -S -I . locale.c
gccmvs -DUSE_MEMMGR -S -I . math.c
gccmvs -DUSE_MEMMGR -S -I . setjmp.c
gccmvs -DUSE_MEMMGR -S -I . signal.c
gccmvs -DUSE_MEMMGR -S -I . __memmgr.c
gccmvs -DUSE_MEMMGR -S -I . pdptest.c
gccmvs -DUSE_MEMMGR -S -I . copyfile.c
gccmvs -DUSE_MEMMGR -S -I . hexdump.c
gccmvs -DUSE_MEMMGR -S -I . mvsendec.c
gccmvs -DUSE_MEMMGR -S -I . mvsunzip.c
m4 -I . pdpmus.m4 >pdpmus.job
rem call sub pdpmus.job
call runmus pdpmus.job output.txt
