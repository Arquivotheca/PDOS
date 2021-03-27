gcc -fno-common -c -I../pdpclib strt32.s pdos.c bos.c fat.c exeload.c
gcc -fno-common -c -I../pdpclib physmem.c vmm.c liballoc.c process.c
gcc -fno-common -c -I../pdpclib int21.c int80.c log.c helper.c
gcc -fno-common -c -I../pdpclib patmat.c support.s memmgr.c protintp.c protints.s pdoss.s
gcc -fno-common -c -I../pdpclib vgatext.c
if exist os.a del os.a
ar -r os.a bos.o fat.o exeload.o
ar -r os.a physmem.o vmm.o liballoc.o process.o
ar -r os.a int21.o int80.o log.o helper.o
ar -r os.a memmgr.o patmat.o support.o protintp.o protints.o pdoss.o
ar -r os.a vgatext.o
ld -r -s -o pdos strt32.o pdos.o os.a ../pdpclib/pdos.a
del *.o
del os.a
if exist pdos.exe del pdos.exe
ren pdos pdos.exe
