gcc -fno-common -c -I../pdpclib strt32.s pdos.c bos.c fat.c exeload.c
gcc -fno-common -c -I../pdpclib patmat.c support.s memmgr.c protintp.c protints.s pdoss.s
if exist os.a del os.a
ar -r os.a bos.o fat.o exeload.o
ar -r os.a memmgr.o patmat.o support.o protintp.o protints.o pdoss.o
ld -r -s -o pdos strt32.o pdos.o os.a ../pdpclib/pdos.a
del *.o
del os.a
if exist pdos.exe del pdos.exe
ren pdos pdos.exe
