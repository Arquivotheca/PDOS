del pdpvse.zip
gccmvs -DUSE_MEMMGR -Os -D__VSE__ -S -I . start.c
gccmvs -DUSE_MEMMGR -Os -D__VSE__ -S -I . stdio.c
gccmvs -DUSE_MEMMGR -Os -D__VSE__ -S -I . stdlib.c
gccmvs -DUSE_MEMMGR -Os -D__VSE__ -S -I . ctype.c
gccmvs -DUSE_MEMMGR -Os -D__VSE__ -S -I . string.c
gccmvs -DUSE_MEMMGR -Os -D__VSE__ -S -I . time.c
gccmvs -DUSE_MEMMGR -Os -D__VSE__ -S -I . errno.c
gccmvs -DUSE_MEMMGR -Os -D__VSE__ -S -I . assert.c
gccmvs -DUSE_MEMMGR -Os -D__VSE__ -S -I . locale.c
gccmvs -DUSE_MEMMGR -Os -D__VSE__ -S -I . math.c
gccmvs -DUSE_MEMMGR -Os -D__VSE__ -S -I . setjmp.c
gccmvs -DUSE_MEMMGR -Os -D__VSE__ -S -I . signal.c
gccmvs -DUSE_MEMMGR -Os -D__VSE__ -S -I . __memmgr.c
gccmvs -DUSE_MEMMGR -Os -D__VSE__ -S -I . pdptest.c
rem zip -0X pdpvse *.s *.exec *.asm *.mac

m4 -I . pdpvse.m4 >pdpvse.jcl
rem call sub pdpvse.jcl
call runvse pdpvse.jcl output.txt
