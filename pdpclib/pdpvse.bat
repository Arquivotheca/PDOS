del pdpvse.zip
gccmvs -Os -D__VSE__ -S -I . -o - start.c | namcsect START >start.s
gccmvs -Os -D__VSE__ -S -I . -o - stdio.c | namcsect @@STDIO >stdio.s
gccmvs -Os -D__VSE__ -S -I . -o - stdlib.c | namcsect @@STDLIB >stdlib.s
gccmvs -Os -D__VSE__ -S -I . -o - ctype.c | namcsect @@CTYPE >ctype.s
gccmvs -Os -D__VSE__ -S -I . -o - string.c | namcsect @@STRING >string.s
gccmvs -Os -D__VSE__ -S -I . -o - time.c | namcsect @@PDPTIM >time.s
gccmvs -Os -D__VSE__ -S -I . -o - errno.c | namcsect @@PDPER >errno.s
gccmvs -Os -D__VSE__ -S -I . -o - assert.c | namcsect @@PDPASS >assert.s
gccmvs -Os -D__VSE__ -S -I . -o - locale.c | namcsect @@PDPLOC >locale.s
gccmvs -Os -D__VSE__ -S -I . -o - math.c | namcsect @@MATH >math.s
gccmvs -Os -D__VSE__ -S -I . -o - setjmp.c | namcsect @@PDPJMP >setjmp.s
gccmvs -Os -D__VSE__ -S -I . -o - signal.c | namcsect @@PDPSIG >signal.s
gccmvs -Os -D__VSE__ -S -I . -o - __memmgr.c | namcsect @@PDPMEM >__memmgr.s
gccmvs -Os -D__VSE__ -S -I . -o - pdptest.c | namcsect PDPTEST >pdptest.s
rem zip -0X pdpvse *.s *.exec *.asm *.mac

m4 -I . pdpvse.m4 >pdpvse.jcl
rem call sub pdpvse.jcl
call runvse pdpvse.jcl output.txt
