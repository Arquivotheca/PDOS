gccmvs -DXXX_MEMMGR -S -I . start.c
gccmvs -DXXX_MEMMGR -S -I . stdio.c
gccmvs -DXXX_MEMMGR -S -I . stdlib.c
gccmvs -DXXX_MEMMGR -S -I . ctype.c
gccmvs -DXXX_MEMMGR -S -I . string.c
gccmvs -DXXX_MEMMGR -S -I . time.c
gccmvs -DXXX_MEMMGR -S -I . errno.c
gccmvs -DXXX_MEMMGR -S -I . assert.c
gccmvs -DXXX_MEMMGR -S -I . locale.c
gccmvs -DXXX_MEMMGR -S -I . math.c
gccmvs -DXXX_MEMMGR -S -I . setjmp.c
gccmvs -DXXX_MEMMGR -S -I . signal.c
gccmvs -DXXX_MEMMGR -S -I . __memmgr.c
gccmvs -DXXX_MEMMGR -S -I . pdptest.c
m4 -I . pdpmvs.m4 >pdpmvs.jcl
rem call sub pdpmvs.jcl
call runmvs pdpmvs.jcl output.txt
