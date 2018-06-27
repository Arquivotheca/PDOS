gcc -fno-common -c -I../pdpclib strt32.s pdos.c bos.c fat.c
gcc -fno-common -c -I../pdpclib patmat.c support.s memmgr.c protintp.c protints.s pdoss.s
ld -r -s -o pdos strt32.o pdos.o bos.o fat.o memmgr.o patmat.o support.o protintp.o protints.o pdoss.o ../pdpclib/pdos.a
if exist pdos.exe del pdos.exe
ren pdos pdos.exe
