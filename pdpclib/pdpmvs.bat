gccmvs -S -I . start.c
gccmvs -S -I . stdio.c
gccmvs -S -I . stdlib.c
gccmvs -S -I . ctype.c
gccmvs -S -I . string.c
gccmvs -S -I . time.c
gccmvs -S -I . errno.c
gccmvs -S -I . assert.c
gccmvs -S -I . locale.c
gccmvs -S -I . math.c
gccmvs -S -I . setjmp.c
gccmvs -S -I . signal.c
gccmvs -S -I . pdptest.c
m4 -I . pdpmvs.m4 >pdpmvs.jcl
rem call sub pdpmvs.jcl
call runmvs pdpmvs.jcl output.txt
