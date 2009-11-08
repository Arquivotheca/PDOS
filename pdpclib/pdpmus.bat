rem Update pdptop.mac if you don't want the default S/380.
rem If choosing S/370 you should also switch off MEMMGR

gccmvs -DUSE_MEMMGR -DMUSIC -S -I . start.c
gccmvs -DUSE_MEMMGR -DMUSIC -S -I . stdio.c
gccmvs -DUSE_MEMMGR -DMUSIC -S -I . stdlib.c
gccmvs -DUSE_MEMMGR -DMUSIC -S -I . ctype.c
gccmvs -DUSE_MEMMGR -DMUSIC -S -I . string.c
gccmvs -DUSE_MEMMGR -DMUSIC -S -I . time.c
gccmvs -DUSE_MEMMGR -DMUSIC -S -I . errno.c
gccmvs -DUSE_MEMMGR -DMUSIC -S -I . assert.c
gccmvs -DUSE_MEMMGR -DMUSIC -S -I . locale.c
gccmvs -DUSE_MEMMGR -DMUSIC -S -I . math.c
gccmvs -DUSE_MEMMGR -DMUSIC -S -I . setjmp.c
gccmvs -DUSE_MEMMGR -DMUSIC -S -I . signal.c
gccmvs -DUSE_MEMMGR -DMUSIC -S -I . __memmgr.c
gccmvs -DUSE_MEMMGR -DMUSIC -S -I . pdptest.c
gccmvs -DUSE_MEMMGR -DMUSIC -S -I . copyfile.c
gccmvs -DUSE_MEMMGR -DMUSIC -S -I . hexdump.c
gccmvs -DUSE_MEMMGR -DMUSIC -S -I . mvsendec.c
gccmvs -DUSE_MEMMGR -DMUSIC -S -I . mvsunzip.c
m4 -I . pdpmus.m4 >pdpmus.job
rem call sub pdpmus.job
call runmus pdpmus.job output.txt
