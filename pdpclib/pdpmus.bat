rem update pdptop.mac to specify MUSIC before running this
rem also choose between S/380 and S/370. If choosing S/370,
rem you should not use MEMMGR

gccmvs -DLOCMODE -DUSE_MEMMGR -S -I . start.c
gccmvs -DLOCMODE -DUSE_MEMMGR -S -I . stdio.c
gccmvs -DLOCMODE -DUSE_MEMMGR -S -I . stdlib.c
gccmvs -DLOCMODE -DUSE_MEMMGR -S -I . ctype.c
gccmvs -DLOCMODE -DUSE_MEMMGR -S -I . string.c
gccmvs -DLOCMODE -DUSE_MEMMGR -S -I . time.c
gccmvs -DLOCMODE -DUSE_MEMMGR -S -I . errno.c
gccmvs -DLOCMODE -DUSE_MEMMGR -S -I . assert.c
gccmvs -DLOCMODE -DUSE_MEMMGR -S -I . locale.c
gccmvs -DLOCMODE -DUSE_MEMMGR -S -I . math.c
gccmvs -DLOCMODE -DUSE_MEMMGR -S -I . setjmp.c
gccmvs -DLOCMODE -DUSE_MEMMGR -S -I . signal.c
gccmvs -DLOCMODE -DUSE_MEMMGR -S -I . __memmgr.c
gccmvs -DLOCMODE -DUSE_MEMMGR -S -I . pdptest.c
m4 -I . pdpmus.m4 >pdpmus.job
rem call sub pdpmus.job
call runmus pdpmus.job output.txt
