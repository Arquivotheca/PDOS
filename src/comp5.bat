gcc -c -I../pdpclib strt32.s pdos.c bos.c fat.c ../pdpclib/string.c ../pdpclib/ctype.c
gcc -c -I../pdpclib patmat.c support.s memmgr.c protintp.c protints.s pdoss.s
ld -s -o pdos strt32.o pdos.o bos.o fat.o memmgr.o patmat.o support.o protintp.o protints.o pdoss.o ../pdpclib/pdos.a
del pdos.exe
ren pdos pdos.exe
