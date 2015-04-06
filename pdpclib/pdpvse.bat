del pdpvse.zip
gccvse -DUSE_MEMMGR -ansi -Os -S -I . start.c
gccvse -DUSE_MEMMGR -ansi -Os -S -I . stdio.c
gccvse -DUSE_MEMMGR -ansi -Os -S -I . stdlib.c
gccvse -DUSE_MEMMGR -ansi -Os -S -I . ctype.c
gccvse -DUSE_MEMMGR -ansi -Os -S -I . string.c
gccvse -DUSE_MEMMGR -ansi -Os -S -I . time.c
gccvse -DUSE_MEMMGR -ansi -Os -S -I . errno.c
gccvse -DUSE_MEMMGR -ansi -Os -S -I . assert.c
gccvse -DUSE_MEMMGR -ansi -Os -S -I . locale.c
gccvse -DUSE_MEMMGR -ansi -Os -S -I . math.c
gccvse -DUSE_MEMMGR -ansi -Os -S -I . setjmp.c
gccvse -DUSE_MEMMGR -ansi -Os -S -I . signal.c
gccvse -DUSE_MEMMGR -ansi -Os -S -I . __memmgr.c
gccvse -DUSE_MEMMGR -ansi -Os -S -I . pdptest.c
rem zip -0X pdpvse *.s *.exec *.asm *.mac

m4 -I . pdpvse.m4 >pdpvse.jcl
rem call sub pdpvse.jcl
sleep 2
call runvse pdpvse.jcl output.txt
