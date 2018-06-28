gcc386 -S -Os -fno-common -I../pdpclib -D__32BIT__ pdos.c
gcc386 -S -Os -fno-common -I../pdpclib -D__32BIT__ bos.c
gcc386 -S -Os -fno-common -I../pdpclib -D__32BIT__ fat.c
gcc386 -S -Os -fno-common -I../pdpclib -D__32BIT__ patmat.c
gcc386 -S -Os -fno-common -I../pdpclib -D__32BIT__ memmgr.c
gcc386 -S -Os -fno-common -I../pdpclib -D__32BIT__ protintp.c

as386 -o strt32.o strt32.s
as386 -o support.o support.s
as386 -o protints.o protints.s
as386 -o pdoss.o pdoss.s
as386 -o pdos.o pdos.s
as386 -o bos.o bos.s
as386 -o fat.o fat.s
as386 -o patmat.o patmat.s
as386 -o memmgr.o memmgr.s
as386 -o protintp.o protintp.s

ld386 -s -e start --oformat a.out-i386 -o pdos.exe strt32.o pdos.o bos.o fat.o memmgr.o patmat.o support.o protintp.o protints.o pdoss.o ../pdpclib/pdos.a
ld386 -r -s -e start --oformat a.out-i386 -o pdos.exe strt32.o pdos.o bos.o fat.o memmgr.o patmat.o support.o protintp.o protints.o pdoss.o ../pdpclib/pdos.a
strip386 --strip-unneeded pdos.exe
