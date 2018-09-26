gcc386 -S -Os -fno-common -I../pdpclib pdos.c
gcc386 -S -Os -fno-common -I../pdpclib bos.c
gcc386 -S -Os -fno-common -I../pdpclib fat.c
gcc386 -S -Os -fno-common -I../pdpclib patmat.c
gcc386 -S -Os -fno-common -I../pdpclib memmgr.c
gcc386 -S -Os -fno-common -I../pdpclib protintp.c

as386 -o pdos.o pdos.s
del pdos.s
as386 -o bos.o bos.s
del bos.s
as386 -o fat.o fat.s
del fat.s
as386 -o patmat.o patmat.s
del patmat.s
as386 -o memmgr.o memmgr.s
del memmgr.s
as386 -o protintp.o protintp.s
del protintp.s

as386 -o strt32.o strt32.s
as386 -o support.o support.s
as386 -o protints.o protints.s
as386 -o pdoss.o pdoss.s

ld386 -s -e start --oformat a.out-i386 -o pdos.exe strt32.o pdos.o bos.o fat.o memmgr.o patmat.o support.o protintp.o protints.o pdoss.o ../pdpclib/pdos.a
ld386 -r -s -e start --oformat a.out-i386 -o pdos.exe strt32.o pdos.o bos.o fat.o memmgr.o patmat.o support.o protintp.o protints.o pdoss.o ../pdpclib/pdos.a
strip386 --strip-unneeded pdos.exe
